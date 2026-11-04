#include "AudioGraph.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <atomic>
#include <cmath>

namespace dash::core
{

struct AudioGraph::Impl
{
    double sampleRate     = 48000.0;
    int    maxBlockSize   = 512;
    int    activeChannels = 2;

    AudioGraphParameters params {};
    std::atomic<bool>  bypassAll { false };
    std::atomic<float> preampLin { 1.0f };

    ParametricEQProcessor parametricEq;
    LimiterProcessor      limiter;

    void updatePreamp() noexcept
    {
        preampLin.store(
            juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, params.preampDb)),
            std::memory_order_relaxed);
    }
};

AudioGraph::AudioGraph()
    : impl_(std::make_unique<Impl>())
{
    impl_->updatePreamp();
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

    impl_->parametricEq.prepare(safeSr, safeBlock, safeCh);
    impl_->limiter.prepare(safeSr, safeBlock, safeCh);

    impl_->updatePreamp();
    for (size_t b = 0; b < ParametricEQProcessor::kMaxBands; ++b)
        impl_->parametricEq.setBandGainDb(b, impl_->params.bandGainsDb[b]);

    impl_->limiter.updateParameters(impl_->params.limiter);
    reset();
}

void AudioGraph::reset() noexcept
{
    impl_->parametricEq.reset();
    impl_->limiter.reset();
}

void AudioGraph::updateParameters(const AudioGraphParameters& parameters) noexcept
{
    impl_->params = parameters;
    impl_->bypassAll.store(parameters.bypassAll, std::memory_order_relaxed);
    impl_->updatePreamp();

    for (size_t b = 0; b < ParametricEQProcessor::kMaxBands; ++b)
        impl_->parametricEq.setBandGainDb(b, impl_->params.bandGainsDb[b]);

    impl_->parametricEq.setBypass(false);
    impl_->limiter.updateParameters(impl_->params.limiter);
}

const AudioGraphParameters& AudioGraph::getParameters() const noexcept
{
    return impl_->params;
}

void AudioGraph::setBandGainDb(size_t bandIndex, float gainDb) noexcept
{
    if (bandIndex >= ParametricEQProcessor::kMaxBands)
        return;

    const float clamped = juce::jlimit(-12.0f, 12.0f, gainDb);
    impl_->params.bandGainsDb[bandIndex] = clamped;
    impl_->parametricEq.setBandGainDb(bandIndex, clamped);
}

void AudioGraph::setPreampDb(float preampDb) noexcept
{
    impl_->params.preampDb = juce::jlimit(-12.0f, 12.0f, preampDb);
    impl_->updatePreamp();
}

void AudioGraph::setBypass(bool enabled) noexcept
{
    impl_->params.bypassAll = enabled;
    impl_->bypassAll.store(enabled, std::memory_order_relaxed);
}

bool AudioGraph::isBypassed() const noexcept
{
    return impl_->bypassAll.load(std::memory_order_relaxed);
}

void AudioGraph::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    if (impl_->bypassAll.load(std::memory_order_relaxed))
        return;

    juce::ScopedNoDenormals noDenormals;

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));
    const int safeCh      = static_cast<int>(activeCh);

    // 1. Etapa de Ganancia Previa (Preamp)
    const float pGain = impl_->preampLin.load(std::memory_order_relaxed);
    if (std::abs(pGain - 1.0f) > 1.0e-4f)
    {
        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            float* chData = channelPointers[ch];
            if (chData != nullptr)
            {
                for (int i = 0; i < numSamples; ++i)
                    chData[i] *= pGain;
            }
        }
    }

    // 2. Ecualizador Táctico Estéreo de 6 Bandas
    impl_->parametricEq.process(channelPointers, safeCh, numSamples);

    // 3. Sample Peak Limiter (-0.5 dBFS) de latencia cero
    impl_->limiter.process(channelPointers, safeCh, numSamples);
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

float AudioGraph::getLimiterReductionDb() const noexcept
{
    return impl_->limiter.getCurrentReductionDb();
}

} // namespace dash::core
