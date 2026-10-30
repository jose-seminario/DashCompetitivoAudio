#include "BinauralRenderer.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>

namespace dash::spatial
{

struct BinauralRenderer::Impl
{
    double sampleRate   = 48000.0;
    int    maxBlockSize = 512;
    BinauralRendererParameters params {};

    ConvolutionEngine convolution;

    // Filtro pasa-bajos acústico de segundo orden a 120 Hz para el canal LFE
    juce::dsp::IIR::Filter<float> lfeFilter;

    // Filtros de Crossfeed para auriculares
    juce::dsp::IIR::Filter<float> crossfeedL;
    juce::dsp::IIR::Filter<float> crossfeedR;

    float masterGainLin = 1.0f;
    float lfeGainLin    = 0.7071f;

    void updateGains() noexcept
    {
        masterGainLin = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 12.0f, params.masterGainDb));
        lfeGainLin    = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 6.0f, params.lfeGainDb)) * 0.7071f;
    }
};

BinauralRenderer::BinauralRenderer()
    : impl_(std::make_unique<Impl>())
{
    impl_->updateGains();
}

BinauralRenderer::~BinauralRenderer() = default;
BinauralRenderer::BinauralRenderer(BinauralRenderer&&) noexcept = default;
BinauralRenderer& BinauralRenderer::operator=(BinauralRenderer&&) noexcept = default;

void BinauralRenderer::prepare(double sampleRate, int maximumBlockSize)
{
    impl_->sampleRate   = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    impl_->maxBlockSize = std::max(1, maximumBlockSize);

    impl_->convolution.prepare(impl_->sampleRate, impl_->maxBlockSize, 512);

    // Preparar filtro pasa-bajos de 120 Hz para LFE
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = impl_->sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(impl_->maxBlockSize);
    spec.numChannels      = 1;

    impl_->lfeFilter.prepare(spec);
    impl_->lfeFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        impl_->sampleRate, 120.0f, 0.7071f);

    impl_->crossfeedL.prepare(spec);
    impl_->crossfeedR.prepare(spec);
    impl_->crossfeedL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        impl_->sampleRate, 700.0f, 0.5f);
    impl_->crossfeedR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        impl_->sampleRate, 700.0f, 0.5f);

    impl_->updateGains();
    reset();
}

void BinauralRenderer::reset() noexcept
{
    impl_->convolution.reset();
    impl_->lfeFilter.reset();
    impl_->crossfeedL.reset();
    impl_->crossfeedR.reset();
}

bool BinauralRenderer::setDataset(const HRIRDataset& dataset, const ChannelLayout& layout)
{
    layout_ = layout;
    const size_t numChannels = layout.getNumChannels();

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        const auto& spk = layout.getSpeaker(ch);

        // Si es LFE y se bypassa HRTF, no cargamos filtro en la convolución
        if (spk.role == ChannelRole::LFE && impl_->params.bypassLfeHrtf)
            continue;

        const auto* pair = dataset.getImpulsePair(ch);
        if (pair != nullptr && pair->isValid())
        {
            impl_->convolution.loadFilterPair(
                ch,
                pair->leftEar.data(),
                pair->rightEar.data(),
                pair->leftEar.size());
        }
    }

    impl_->convolution.commitFilters();
    isConfigured_ = true;
    return true;
}

void BinauralRenderer::updateParameters(const BinauralRendererParameters& params) noexcept
{
    impl_->params = params;
    impl_->updateGains();
}

const BinauralRendererParameters& BinauralRenderer::getParameters() const noexcept
{
    return impl_->params;
}

void BinauralRenderer::render(
    const float* const* inputChannels,
    size_t numChannels,
    float* outputLeft,
    float* outputRight,
    int numSamples) noexcept
{
    if (inputChannels == nullptr || outputLeft == nullptr || outputRight == nullptr || numSamples <= 0)
        return;

    // Inicializar los búferes de salida en silencio
    std::fill(outputLeft, outputLeft + numSamples, 0.0f);
    std::fill(outputRight, outputRight + numSamples, 0.0f);

    if (impl_->params.bypass)
    {
        // En bypass, pasar estéreo frontal L/R directo
        if (numChannels >= 1 && inputChannels[0] != nullptr)
            std::copy(inputChannels[0], inputChannels[0] + numSamples, outputLeft);
        if (numChannels >= 2 && inputChannels[1] != nullptr)
            std::copy(inputChannels[1], inputChannels[1] + numSamples, outputRight);
        return;
    }

    const size_t activeCh = std::min(numChannels, layout_.getNumChannels());

    for (size_t ch = 0; ch < activeCh; ++ch)
    {
        const float* in = inputChannels[ch];
        if (in == nullptr)
            continue;

        const auto& spk = layout_.getSpeaker(ch);

        // Tratamiento especializado de Subwoofer LFE
        if (spk.role == ChannelRole::LFE && impl_->params.bypassLfeHrtf)
        {
            const float lfeGain = impl_->lfeGainLin;
            for (int i = 0; i < numSamples; ++i)
            {
                const float filteredLfe = impl_->lfeFilter.processSample(in[i]) * lfeGain;
                outputLeft[i]  += filteredLfe;
                outputRight[i] += filteredLfe;
            }
            continue;
        }

        // Convolución espacial tridimensional HRIR
        impl_->convolution.convolveAndAccumulate(ch, in, outputLeft, outputRight, numSamples);
    }

    // Procesamiento opcional de Crossfeed de auriculares
    if (impl_->params.crossfeedAmount > 0.001f)
    {
        const float cfGain = impl_->params.crossfeedAmount * 0.35f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float cfL = impl_->crossfeedL.processSample(outputLeft[i]) * cfGain;
            const float cfR = impl_->crossfeedR.processSample(outputRight[i]) * cfGain;
            outputLeft[i]  += cfR;
            outputRight[i] += cfL;
        }
    }

    // Aplicación de ganancia maestra binaural
    const float masterG = impl_->masterGainLin;
    if (std::abs(masterG - 1.0f) > 1.0e-4f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            outputLeft[i]  *= masterG;
            outputRight[i] *= masterG;
        }
    }
}

} // namespace dash::spatial
