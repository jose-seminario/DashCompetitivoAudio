#include "EQProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>

namespace dash::core
{

std::array<double, EQProcessor::kNumBands> EQProcessor::computeBandGainsDb(
    double footstepsDb,
    double bassDb) noexcept
{
    std::array<double, kNumBands> gains = kBaseCurveDb;

    const double deltaBass  = bassDb - 1.5;
    const double deltaSteps = footstepsDb - 1.6;

    // Banda Graves: 80 Hz hombro 50%, 100..250 Hz 100%
    gains[6]  += deltaBass * 0.5;
    gains[7]  += deltaBass;
    gains[8]  += deltaBass;
    gains[9]  += deltaBass;
    gains[10] += deltaBass;
    gains[11] += deltaBass;

    // Banda Pasos: 3150 Hz (+1.6) y 6300 Hz (+2.2) + hombros acústicos suaves
    gains[21] += deltaSteps * 0.50;
    gains[22]  = footstepsDb;
    gains[23] += deltaSteps * 0.45;
    gains[25]  = 2.2 + deltaSteps * (2.2 / 1.6);
    gains[26] += deltaSteps * 0.55;

    return gains;
}

struct EQProcessor::Impl
{
    static constexpr float kThirdOctaveQ = 4.318477f;

    double sampleRate = 48000.0;
    bool   isPrepared = false;
    EQParameters params {};

    std::atomic<bool> atomicBypass { false };
    std::atomic<float> atomicPreampLin { 1.0f };
    std::atomic<bool> pendingCoeffUpdate { false };

    float cachedFootstepsDb = -999.0f;
    float cachedBassDb      = -999.0f;

    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, kNumBands> sharedBandCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, kNumBands> stagingBandCoeffs;
    std::array<std::array<juce::dsp::IIR::Filter<float>, kNumBands>, kMaxChannels> filters;
    std::array<bool, kNumBands> activeBandMask {};
    std::array<bool, kNumBands> stagingActiveMask {};

    void refreshFilterCoefficients() noexcept
    {
        atomicPreampLin.store(juce::Decibels::decibelsToGain(params.preampDb), std::memory_order_relaxed);
        atomicBypass.store(params.bypass, std::memory_order_relaxed);

        if (!isPrepared)
            return;

        if (std::abs(params.footstepsDb - cachedFootstepsDb) <= 1.0e-4f
            && std::abs(params.bassDb - cachedBassDb) <= 1.0e-4f)
        {
            return;
        }

        cachedFootstepsDb = params.footstepsDb;
        cachedBassDb      = params.bassDb;

        const auto gainsDb = EQProcessor::computeBandGainsDb(
            static_cast<double>(cachedFootstepsDb),
            static_cast<double>(cachedBassDb));
        const float nyquistLimit = static_cast<float>(sampleRate * 0.48);

        for (size_t b = 0; b < kNumBands; ++b)
        {
            const float freqHz = kIsoFrequencies[b];
            const float gainDb = static_cast<float>(gainsDb[b]);

            if (freqHz >= nyquistLimit || std::abs(gainDb) < 1.0e-3f)
            {
                stagingActiveMask[b] = false;
                continue;
            }

            stagingActiveMask[b] = true;
            const float gainFactor = juce::Decibels::decibelsToGain(gainDb);
            stagingBandCoeffs[b] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, freqHz, kThirdOctaveQ, gainFactor);
        }

        pendingCoeffUpdate.store(true, std::memory_order_release);
    }
};

EQProcessor::EQProcessor()
    : impl_(std::make_unique<Impl>())
{
}

EQProcessor::~EQProcessor() = default;
EQProcessor::EQProcessor(EQProcessor&&) noexcept = default;
EQProcessor& EQProcessor::operator=(EQProcessor&&) noexcept = default;

void EQProcessor::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    (void)numChannels;
    impl_->sampleRate = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    const auto safeBlock = static_cast<juce::uint32>(std::max(1, maximumBlockSize));

    juce::dsp::ProcessSpec monoSpec {};
    monoSpec.sampleRate       = impl_->sampleRate;
    monoSpec.maximumBlockSize = safeBlock;
    monoSpec.numChannels      = 1;

    const float nyquistLimit = static_cast<float>(impl_->sampleRate * 0.48);

    // Pre-asignar una sola vez los 31 objetos Coefficients fuera del hilo de audio
    for (size_t b = 0; b < kNumBands; ++b)
    {
        const float safeFreq = std::min(kIsoFrequencies[b], nyquistLimit);
        impl_->sharedBandCoeffs[b] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            impl_->sampleRate, safeFreq, Impl::kThirdOctaveQ, 1.0f);
        impl_->stagingBandCoeffs[b] = impl_->sharedBandCoeffs[b];
    }

    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        for (size_t b = 0; b < kNumBands; ++b)
        {
            impl_->filters[ch][b].prepare(monoSpec);
            impl_->filters[ch][b].coefficients = impl_->sharedBandCoeffs[b];
        }
    }

    impl_->isPrepared        = true;
    impl_->cachedFootstepsDb = -999.0f;
    impl_->cachedBassDb      = -999.0f;
    impl_->refreshFilterCoefficients();

    // Sincronizar inmediatamente al preparar
    if (impl_->pendingCoeffUpdate.load(std::memory_order_acquire))
    {
        for (size_t b = 0; b < kNumBands; ++b)
        {
            impl_->activeBandMask[b] = impl_->stagingActiveMask[b];
            if (impl_->stagingBandCoeffs[b] != nullptr)
            {
                impl_->sharedBandCoeffs[b] = impl_->stagingBandCoeffs[b];
                for (size_t ch = 0; ch < kMaxChannels; ++ch)
                    impl_->filters[ch][b].coefficients = impl_->sharedBandCoeffs[b];
            }
        }
        impl_->pendingCoeffUpdate.store(false, std::memory_order_release);
    }

    reset();
}

void EQProcessor::reset() noexcept
{
    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        for (size_t b = 0; b < kNumBands; ++b)
            impl_->filters[ch][b].reset();
    }
}

void EQProcessor::updateParameters(const EQParameters& params) noexcept
{
    impl_->params = params;
    impl_->refreshFilterCoefficients();
}

void EQProcessor::process(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    float* channels[1] = { buffer };
    process(channels, 1, numSamples);
}

void EQProcessor::process(std::vector<float>& buffer) noexcept
{
    if (buffer.empty())
        return;

    process(buffer.data(), static_cast<int>(buffer.size()));
}

void EQProcessor::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0 || impl_->atomicBypass.load(std::memory_order_relaxed))
        return;

    juce::ScopedNoDenormals noDenormals;

    // Conmutación lock-free de coeficientes exclusivamente en los límites entre bloques de audio
    if (impl_->pendingCoeffUpdate.load(std::memory_order_acquire))
    {
        for (size_t b = 0; b < kNumBands; ++b)
        {
            impl_->activeBandMask[b] = impl_->stagingActiveMask[b];
            if (impl_->stagingBandCoeffs[b] != nullptr)
            {
                impl_->sharedBandCoeffs[b] = impl_->stagingBandCoeffs[b];
                for (size_t ch = 0; ch < kMaxChannels; ++ch)
                    impl_->filters[ch][b].coefficients = impl_->sharedBandCoeffs[b];
            }
        }
        impl_->pendingCoeffUpdate.store(false, std::memory_order_release);
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));
    const float preamp    = impl_->atomicPreampLin.load(std::memory_order_relaxed);

    for (size_t ch = 0; ch < activeCh; ++ch)
    {
        float* chData = channelPointers[ch];
        if (chData == nullptr)
            continue;

        auto& chFilters = impl_->filters[ch];
        for (int i = 0; i < numSamples; ++i)
        {
            float s = chData[i] * preamp;
            for (size_t b = 0; b < kNumBands; ++b)
            {
                if (impl_->activeBandMask[b])
                    s = chFilters[b].processSample(s);
            }
            chData[i] = s;
        }
    }
}

} // namespace dash::core
