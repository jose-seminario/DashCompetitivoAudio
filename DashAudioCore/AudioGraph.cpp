#include "AudioGraph.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace dash::core
{

struct AudioGraph::Impl
{
    double sampleRate     = 48000.0;
    int    maxBlockSize   = 512;
    int    activeChannels = 2;

    AudioGraphParameters params {};
    float headroomLin = 0.50118723f; // -6 dBFS

    DynamicsProcessor     dynamics;
    TransientProcessor    transient;
    AmbienceProcessor     ambience;
    EQProcessor           equalizer;
    ParametricEQProcessor parametricEq;
    LimiterProcessor      limiter;

    void updateHeadroom() noexcept
    {
        headroomLin = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 6.0f, params.headroomDb));
    }
};

AudioGraph::AudioGraph()
    : impl_(std::make_unique<Impl>())
{
    impl_->updateHeadroom();
}

AudioGraph::~AudioGraph() = default;
AudioGraph::AudioGraph(AudioGraph&&) noexcept = default;
AudioGraph& AudioGraph::operator=(AudioGraph&&) noexcept = default;

void AudioGraph::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    const double safeSr = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    const int safeBlock = std::max(1, maximumBlockSize);
    const int safeCh    = std::clamp(numChannels, 1, static_cast<int>(kMaxChannels));

    impl_->sampleRate     = safeSr;
    impl_->maxBlockSize   = safeBlock;
    impl_->activeChannels = safeCh;

    impl_->dynamics.prepare(safeSr, safeBlock, safeCh);
    impl_->transient.prepare(safeSr, safeBlock, safeCh);
    impl_->ambience.prepare(safeSr, safeBlock, safeCh);
    impl_->equalizer.prepare(safeSr, safeBlock, safeCh);
    impl_->parametricEq.prepare(safeSr, safeBlock, safeCh);
    impl_->limiter.prepare(safeSr, safeBlock, safeCh);

    impl_->updateHeadroom();
    reset();
}

void AudioGraph::reset() noexcept
{
    impl_->dynamics.reset();
    impl_->transient.reset();
    impl_->ambience.reset();
    impl_->equalizer.reset();
    impl_->parametricEq.reset();
    impl_->limiter.reset();
}

void AudioGraph::updateParameters(const AudioGraphParameters& parameters) noexcept
{
    impl_->params = parameters;
    impl_->updateHeadroom();

    impl_->dynamics.updateParameters(impl_->params.dynamics);
    impl_->transient.updateParameters(impl_->params.transient);
    impl_->ambience.updateParameters(impl_->params.ambience);
    impl_->equalizer.updateParameters(impl_->params.equalizer);

    for (size_t b = 0; b < ParametricEQProcessor::kMaxBands; ++b)
        impl_->parametricEq.setBand(b, impl_->params.parametricBands[b]);
    impl_->parametricEq.setBypass(!impl_->params.useParametricEq);

    impl_->limiter.updateParameters(impl_->params.limiter);
}

const AudioGraphParameters& AudioGraph::getParameters() const noexcept
{
    return impl_->params;
}

void AudioGraph::setBypass(bool enabled) noexcept
{
    impl_->params.bypassAll = enabled;
}

bool AudioGraph::isBypassed() const noexcept
{
    return impl_->params.bypassAll;
}

void AudioGraph::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    // Si la cadena completa está en bypass, el audio pasa intacto sin procesamiento
    if (impl_->params.bypassAll)
        return;

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));

    // 1. Etapa de Margen Dinámico / Entrada (Headroom)
    if (std::abs(impl_->params.headroomDb) > 1.0e-3f)
    {
        const float hGain = impl_->headroomLin;
        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            float* chData = channelPointers[ch];
            if (chData != nullptr)
            {
                for (int i = 0; i < numSamples; ++i)
                    chData[i] *= hGain;
            }
        }
    }

    // 2. Compresor Dinámico Híbrido (ITU-R BS.1770-4 Upward + Downward)
    impl_->dynamics.process(channelPointers, numChannels, numSamples);

    // 3. Supresor de Transitorios de Disparo (Canal Central / Gun Crack)
    impl_->transient.process(channelPointers, numChannels, numSamples);

    // 4. Protector de Frecuencias Ambientales (Multibanda 350 Hz - 2.5 kHz)
    impl_->ambience.process(channelPointers, numChannels, numSamples);

    // 5. Ecualización Acústica (31 Bandas ISO y/o Paramétrica de precisión)
    impl_->equalizer.process(channelPointers, numChannels, numSamples);
    if (impl_->params.useParametricEq)
        impl_->parametricEq.process(channelPointers, numChannels, numSamples);

    // 6. Limitador de Picos de Muestra Exacta (-0.5 dBFS)
    impl_->limiter.process(channelPointers, numChannels, numSamples);
}

void AudioGraph::process(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    float* channels[1] = { buffer };
    process(channels, 1, numSamples);
}

void AudioGraph::process(std::vector<float>& buffer) noexcept
{
    if (buffer.empty())
        return;

    process(buffer.data(), static_cast<int>(buffer.size()));
}

float AudioGraph::getLufs() const noexcept
{
    return impl_->dynamics.getCurrentLufs();
}

float AudioGraph::getDynamicsGainDb() const noexcept
{
    return impl_->dynamics.getCurrentGainDb();
}

float AudioGraph::getTransientReductionDb() const noexcept
{
    return impl_->transient.getLastReductionDb();
}

float AudioGraph::getAmbienceGainDb() const noexcept
{
    return impl_->ambience.getCurrentGainDb();
}

float AudioGraph::getStationarityIndex() const noexcept
{
    return impl_->ambience.getStationarityIndex();
}

float AudioGraph::getLimiterReductionDb() const noexcept
{
    return impl_->limiter.getCurrentReductionDb();
}

} // namespace dash::core
