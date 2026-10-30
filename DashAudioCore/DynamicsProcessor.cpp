#include "DynamicsProcessor.h"
#include "LimiterProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace dash::core
{

namespace
{
    /**
     * Ponderación espacial por canal según ITU-R BS.1770-4:
     *  - L, R, C (0, 1, 2): 1.0 (0 dB)
     *  - LFE (3 en configuraciones >= 6 canales): 0.0 (excluido de la medición LUFS)
     *  - Surrounds Ls, Rs, Lrs, Rrs (4..7): 1.41 (~ +1.5 dB)
     */
    [[nodiscard]] float getBs1770ChannelWeight(size_t channelIndex, size_t totalChannels) noexcept
    {
        if (totalChannels >= 6)
        {
            if (channelIndex == 3)
                return 0.0f;
            if (channelIndex >= 4)
                return 1.41f;
        }
        return 1.0f;
    }

    [[nodiscard]] float mapNormToFreqHz(float norm) noexcept
    {
        return 20.0f * std::pow(1000.0f, norm);
    }
} // namespace

struct DynamicsProcessor::Impl
{
    struct KWeightingChannel
    {
        juce::dsp::IIR::Filter<float> preFilter;
        juce::dsp::IIR::Filter<float> rlbFilter;
    };

    double sampleRate = 48000.0;
    DynamicsParameters params {};

    std::array<KWeightingChannel, kMaxChannels> kWeightChannels;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kMaxChannels> scHighPass;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kMaxChannels> scLowPass;
    LimiterProcessor peakLimiter;

    float detAtkCoeff  = 0.99f;
    float detRelCoeff  = 0.999f;
    float gainAtkCoeff = 0.99f;
    float gainRelCoeff = 0.999f;

    float meanSquareEnv  = 1.0e-8f;
    float smoothedGainDb = 0.0f;
    float lastLufs       = -70.0f;
    float lastGainDb     = 0.0f;

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    [[nodiscard]] float processKWeightedSample(size_t ch, float inputSample) noexcept
    {
        auto& kw = kWeightChannels[ch];
        const float afterStage1 = kw.preFilter.processSample(inputSample);
        return kw.rlbFilter.processSample(afterStage1);
    }

    [[nodiscard]] float computeHybridTargetGainDb(float currentLufs) const noexcept
    {
        const float amount = juce::jlimit(0.0f, 1.0f, params.amount);
        if (amount <= 0.0001f)
            return 0.0f;

        const float deltaLufs = currentLufs - params.targetLufs;

        if (deltaLufs > 0.0f)
        {
            // Rama Downward Compression: atenúa explosiones/disparos que superan Target (Ratio máx ~ 4:1)
            constexpr float kMaxDownwardSlope = 0.75f;
            return -deltaLufs * (kMaxDownwardSlope * amount);
        }

        // Rama Upward Compression: eleva micro-detalles (pasos, recargas) bajo Target con puerta suave
        if (currentLufs <= params.gateFloorDb)
            return 0.0f;

        constexpr float kGateKneeWidthDb = 10.0f;
        float gateWeight = 1.0f;
        if (currentLufs < params.gateFloorDb + kGateKneeWidthDb)
        {
            gateWeight = (currentLufs - params.gateFloorDb) / kGateKneeWidthDb;
            gateWeight = gateWeight * gateWeight * (3.0f - 2.0f * gateWeight); // Smoothstep C1
        }

        const float deficitDb = -deltaLufs;
        constexpr float kUpwardRatioSlope = 0.55f;
        const float rawBoostDb = deficitDb * kUpwardRatioSlope * amount;
        const float clampedBoostDb = juce::jmin(rawBoostDb, params.maxLiftDb * amount);

        return clampedBoostDb * gateWeight;
    }

    void recalculateParameters() noexcept
    {
        const float hpHz = mapNormToFreqHz(juce::jlimit(0.0f, 0.95f, params.scLowNorm));
        const float lpHz = juce::jmax(hpHz + 80.0f, mapNormToFreqHz(juce::jlimit(0.05f, 1.0f, params.scHighNorm)));

        const float nyquistGuard = static_cast<float>(sampleRate * 0.48);
        const float safeHp = juce::jlimit(20.0f, nyquistGuard, hpHz);
        const float safeLp = juce::jlimit(80.0f, nyquistGuard, lpHz);

        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            scHighPass[ch].setCutoffFrequency(safeHp);
            scLowPass[ch].setCutoffFrequency(safeLp);
        }

        detAtkCoeff = calcOnePoleCoeff(0.015f);
        const float detRelSec = 0.040f + juce::jlimit(0.0f, 1.0f, params.detRelNorm) * 0.620f;
        detRelCoeff = calcOnePoleCoeff(detRelSec);

        gainAtkCoeff = calcOnePoleCoeff(0.006f);
        const float compRelSec = 0.025f + juce::jlimit(0.0f, 1.0f, params.releaseNorm) * 0.450f;
        gainRelCoeff = calcOnePoleCoeff(compRelSec);

        LimiterParameters limParams {};
        limParams.ceilingDb   = -0.5f;
        limParams.releaseNorm = params.limRelNorm;
        limParams.bypass      = params.bypass;
        peakLimiter.updateParameters(limParams);
    }
};

DynamicsProcessor::DynamicsProcessor()
    : impl_(std::make_unique<Impl>())
{
}

DynamicsProcessor::~DynamicsProcessor() = default;
DynamicsProcessor::DynamicsProcessor(DynamicsProcessor&&) noexcept = default;
DynamicsProcessor& DynamicsProcessor::operator=(DynamicsProcessor&&) noexcept = default;

void DynamicsProcessor::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    const double safeSr = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    const int safeBlock = std::max(1, maximumBlockSize);
    const int clampedCh = std::clamp(numChannels, 1, static_cast<int>(kMaxChannels));

    impl_->sampleRate = safeSr;

    // Etapa 1 ITU-R BS.1770-4 (Pre-filter acústico): High-Shelf +3.99984 dB @ 1681.97 Hz, Q = 0.707175
    auto stage1Coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        safeSr,
        1681.97445f,
        0.707175237f,
        juce::Decibels::decibelsToGain(3.99984385f));

    // Etapa 2 ITU-R BS.1770-4 (Filtro RLB): High-Pass @ 38.13547 Hz, Q = 0.500327
    auto stage2Coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        safeSr,
        38.1354709f,
        0.500327037f);

    juce::dsp::ProcessSpec monoSpec {};
    monoSpec.sampleRate       = safeSr;
    monoSpec.maximumBlockSize = static_cast<juce::uint32>(safeBlock);
    monoSpec.numChannels      = 1;

    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        impl_->kWeightChannels[ch].preFilter.prepare(monoSpec);
        impl_->kWeightChannels[ch].rlbFilter.prepare(monoSpec);
        impl_->kWeightChannels[ch].preFilter.coefficients = stage1Coeffs;
        impl_->kWeightChannels[ch].rlbFilter.coefficients = stage2Coeffs;

        impl_->scHighPass[ch].prepare(monoSpec);
        impl_->scHighPass[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        impl_->scHighPass[ch].setCutoffFrequency(20.0f);

        impl_->scLowPass[ch].prepare(monoSpec);
        impl_->scLowPass[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        impl_->scLowPass[ch].setCutoffFrequency(20000.0f);
    }

    impl_->peakLimiter.prepare(safeSr, safeBlock, clampedCh);
    impl_->recalculateParameters();
    reset();
}

void DynamicsProcessor::reset() noexcept
{
    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        impl_->kWeightChannels[ch].preFilter.reset();
        impl_->kWeightChannels[ch].rlbFilter.reset();
        impl_->scHighPass[ch].reset();
        impl_->scLowPass[ch].reset();
    }

    impl_->peakLimiter.reset();
    impl_->meanSquareEnv  = 1.0e-8f;
    impl_->smoothedGainDb = 0.0f;
    impl_->lastLufs       = -70.0f;
    impl_->lastGainDb     = 0.0f;
}

void DynamicsProcessor::updateParameters(const DynamicsParameters& params) noexcept
{
    impl_->params = params;
    impl_->recalculateParameters();
}

void DynamicsProcessor::process(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    float* channels[1] = { buffer };
    process(channels, 1, numSamples);
}

void DynamicsProcessor::process(std::vector<float>& buffer) noexcept
{
    if (buffer.empty())
        return;

    process(buffer.data(), static_cast<int>(buffer.size()));
}

void DynamicsProcessor::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    if (impl_->params.bypass)
    {
        impl_->lastGainDb = 0.0f;
        return;
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));
    const bool useHp = (impl_->params.scLowNorm > 0.001f);
    const bool useLp = (impl_->params.scHighNorm < 0.999f);

    float blockLufs   = impl_->lastLufs;
    float blockGainDb = impl_->lastGainDb;

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Energía instantánea ponderada K (ITU-R BS.1770-4) + filtrado Sidechain TPT
        float weightedSquareSum = 0.0f;

        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] == nullptr)
                continue;

            const float x = channelPointers[ch][i];
            const float weight = getBs1770ChannelWeight(ch, activeCh);

            if (weight <= 0.0f)
                continue;

            float sc = impl_->processKWeightedSample(ch, x);
            if (useHp)
                sc = impl_->scHighPass[ch].processSample(0, sc);
            if (useLp)
                sc = impl_->scLowPass[ch].processSample(0, sc);

            weightedSquareSum += weight * (sc * sc);
        }

        // 2. Detector RMS con balística asimétrica
        const float detCoeff = (weightedSquareSum > impl_->meanSquareEnv) ? impl_->detAtkCoeff : impl_->detRelCoeff;
        impl_->meanSquareEnv = detCoeff * impl_->meanSquareEnv + (1.0f - detCoeff) * weightedSquareSum;

        // Conversión ITU-R BS.1770-4: L_K = -0.691 + 10 * log10(sum(G_i * z_i))
        const float currentLufs = -0.691f + 10.0f * std::log10(juce::jmax(impl_->meanSquareEnv, 1.0e-12f));
        blockLufs = currentLufs;

        // 3. Computadora de ganancia híbrida (Upward + Downward)
        const float targetGainDb = impl_->computeHybridTargetGainDb(currentLufs);
        const float gainCoeff = (targetGainDb < impl_->smoothedGainDb) ? impl_->gainAtkCoeff : impl_->gainRelCoeff;
        impl_->smoothedGainDb = gainCoeff * impl_->smoothedGainDb + (1.0f - gainCoeff) * targetGainDb;
        blockGainDb = impl_->smoothedGainDb;

        const float compGainLin = juce::Decibels::decibelsToGain(impl_->smoothedGainDb);

        // 4. Limitador de seguridad de muestra exacta a través de LimiterProcessor (-0.5 dBFS)
        float peakPostComp = 0.0f;
        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] == nullptr)
                continue;

            const float amplified = std::abs(channelPointers[ch][i] * compGainLin);
            if (amplified > peakPostComp)
                peakPostComp = amplified;
        }

        const float limiterGainLin = impl_->peakLimiter.computeSampleGainLinear(peakPostComp);
        const float totalGainLin   = compGainLin * limiterGainLin;

        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] != nullptr)
                channelPointers[ch][i] *= totalGainLin;
        }
    }

    impl_->lastLufs   = blockLufs;
    impl_->lastGainDb = blockGainDb + impl_->peakLimiter.getCurrentReductionDb();
}

float DynamicsProcessor::getCurrentLufs() const noexcept
{
    return impl_->lastLufs;
}

float DynamicsProcessor::getCurrentGainDb() const noexcept
{
    return impl_->lastGainDb;
}

} // namespace dash::core
