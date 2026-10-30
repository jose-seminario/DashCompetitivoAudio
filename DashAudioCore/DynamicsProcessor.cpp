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
     * Ponderación espacial multicanal según ITU-R BS.1770-4 (Sección 1.4):
     *  - Canales Frontales L, R, C (0, 1, 2): G_i = 1.0 (0.0 dB)
     *  - Canal LFE Subwoofer (3 en layouts >= 6 canales): G_LFE = 0.0 (estrictamente excluido)
     *  - Canales Surround y Altura (4..15): G_s = 1.41 (~ +1.5 dB de ponderación energética)
     */
    [[nodiscard]] float getBs1770ChannelWeight(size_t channelIndex, size_t totalChannels) noexcept
    {
        if (totalChannels >= 6)
        {
            if (channelIndex == 3) // Canal LFE estándar en layouts 5.1 / 7.1 / inmersivos
                return 0.0f;
            if (channelIndex >= 4) // Surrounds laterales, traseros y de altura
                return 1.41f;
        }
        return 1.0f;
    }

    [[nodiscard]] float mapNormToFreqHz(float norm) noexcept
    {
        return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, norm));
    }
} // namespace

struct DynamicsProcessor::Impl
{
    struct KWeightingChannel
    {
        juce::dsp::IIR::Filter<float> preFilter; // Etapa 1: High-Shelf acústico
        juce::dsp::IIR::Filter<float> rlbFilter; // Etapa 2: Filtro RLB pasa-altos
    };

    double sampleRate = 48000.0;
    DynamicsParameters params {};

    std::array<KWeightingChannel, kMaxSupportedChannels> kWeightChannels;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kMaxSupportedChannels> scHighPass;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kMaxSupportedChannels> scLowPass;
    LimiterProcessor peakLimiter;

    float detAtkCoeff  = 0.99f;
    float detRelCoeff  = 0.999f;
    float gainAtkCoeff = 0.99f;
    float gainRelCoeff = 0.999f;

    float meanSquareEnv    = 1.0e-8f;
    float smoothedGainDb   = 0.0f;
    float lastLufs         = -70.0f;
    float lastGainDb       = 0.0f;
    float lastPeakDb       = -100.0f;
    float lastUpwardDb     = 0.0f;
    float lastDownwardDb   = 0.0f;

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0001f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    [[nodiscard]] float processKWeightedSample(size_t ch, float inputSample) noexcept
    {
        auto& kw = kWeightChannels[ch];
        const float afterStage1 = kw.preFilter.processSample(inputSample);
        return kw.rlbFilter.processSample(afterStage1);
    }

    [[nodiscard]] float computeHybridTargetGainDb(float currentLufs, float& outUpwardDb, float& outDownwardDb) const noexcept
    {
        outUpwardDb = 0.0f;
        outDownwardDb = 0.0f;

        const float target = params.useAdvancedMode ? params.thresholdDb : params.targetLufs;
        const float downAmt = params.useAdvancedMode ? params.downwardAmount : params.amount;
        const float upAmt   = params.useAdvancedMode ? params.upwardAmount   : params.amount;

        if (downAmt <= 0.0001f && upAmt <= 0.0001f)
            return params.makeupGainDb;

        const float deltaLufs = currentLufs - target;
        const float knee = juce::jmax(0.1f, params.kneeWidthDb);
        const float halfKnee = knee * 0.5f;

        // 1. Downward Compression con Soft-Knee cuadrático
        if (deltaLufs > -halfKnee && downAmt > 0.0001f)
        {
            const float ratio = params.useAdvancedMode ? juce::jlimit(1.0f, 20.0f, params.ratio) : 4.0f;
            const float slope = 1.0f - (1.0f / ratio);

            if (deltaLufs < halfKnee)
            {
                const float x = deltaLufs + halfKnee;
                outDownwardDb = -slope * (x * x) / (2.0f * knee) * downAmt;
            }
            else
            {
                outDownwardDb = -slope * deltaLufs * downAmt;
            }
        }

        // 2. Upward Compression con compuerta de ruido acústica
        const float gateFloor = params.useAdvancedMode ? params.gateThresholdDb : params.gateFloorDb;
        if (deltaLufs < 0.0f && currentLufs > gateFloor && upAmt > 0.0001f)
        {
            constexpr float kGateKneeWidthDb = 8.0f;
            float gateWeight = 1.0f;

            if (currentLufs < gateFloor + kGateKneeWidthDb)
            {
                gateWeight = (currentLufs - gateFloor) / kGateKneeWidthDb;
                gateWeight = gateWeight * gateWeight * (3.0f - 2.0f * gateWeight);
            }

            const float deficitDb = -deltaLufs;
            constexpr float kUpwardSlope = 0.55f;
            const float rawBoostDb = deficitDb * kUpwardSlope * upAmt;
            const float maxLift = params.maxLiftDb * upAmt;
            const float clampedBoostDb = juce::jmin(rawBoostDb, maxLift);

            outUpwardDb = clampedBoostDb * gateWeight;
        }

        return outDownwardDb + outUpwardDb + params.makeupGainDb;
    }

    void recalculateParameters() noexcept
    {
        float hpHz = 20.0f;
        float lpHz = 20000.0f;

        if (params.useAdvancedMode)
        {
            hpHz = params.scLowHz;
            lpHz = params.scHighHz;
        }
        else
        {
            hpHz = mapNormToFreqHz(params.scLowNorm);
            lpHz = juce::jmax(hpHz + 80.0f, mapNormToFreqHz(params.scHighNorm));
        }

        const float nyquistGuard = static_cast<float>(sampleRate * 0.48);
        const float safeHp = juce::jlimit(20.0f, nyquistGuard, hpHz);
        const float safeLp = juce::jlimit(80.0f, nyquistGuard, lpHz);

        for (size_t ch = 0; ch < kMaxSupportedChannels; ++ch)
        {
            scHighPass[ch].setCutoffFrequency(safeHp);
            scLowPass[ch].setCutoffFrequency(safeLp);
        }

        if (params.useAdvancedMode)
        {
            detAtkCoeff  = calcOnePoleCoeff(0.010f);
            detRelCoeff  = calcOnePoleCoeff(0.400f);
            gainAtkCoeff = calcOnePoleCoeff(juce::jmax(0.0001f, params.attackMs * 0.001f));
            gainRelCoeff = calcOnePoleCoeff(juce::jmax(0.005f, params.releaseMs * 0.001f));
        }
        else
        {
            detAtkCoeff = calcOnePoleCoeff(0.015f);
            const float detRelSec = 0.040f + juce::jlimit(0.0f, 1.0f, params.detRelNorm) * 0.620f;
            detRelCoeff = calcOnePoleCoeff(detRelSec);

            gainAtkCoeff = calcOnePoleCoeff(0.006f);
            const float compRelSec = 0.025f + juce::jlimit(0.0f, 1.0f, params.releaseNorm) * 0.450f;
            gainRelCoeff = calcOnePoleCoeff(compRelSec);
        }

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
    const int clampedCh = std::clamp(numChannels, 1, static_cast<int>(kMaxSupportedChannels));

    impl_->sampleRate = safeSr;

    auto stage1Coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        safeSr,
        1681.97445f,
        0.707175237f,
        juce::Decibels::decibelsToGain(3.99984385f));

    auto stage2Coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        safeSr,
        38.1354709f,
        0.500327037f);

    juce::dsp::ProcessSpec monoSpec {};
    monoSpec.sampleRate       = safeSr;
    monoSpec.maximumBlockSize = static_cast<juce::uint32>(safeBlock);
    monoSpec.numChannels      = 1;

    for (size_t ch = 0; ch < kMaxSupportedChannels; ++ch)
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
    for (size_t ch = 0; ch < kMaxSupportedChannels; ++ch)
    {
        impl_->kWeightChannels[ch].preFilter.reset();
        impl_->kWeightChannels[ch].rlbFilter.reset();
        impl_->scHighPass[ch].reset();
        impl_->scLowPass[ch].reset();
    }

    impl_->peakLimiter.reset();
    impl_->meanSquareEnv    = 1.0e-8f;
    impl_->smoothedGainDb   = 0.0f;
    impl_->lastLufs         = -70.0f;
    impl_->lastGainDb       = 0.0f;
    impl_->lastPeakDb       = -100.0f;
    impl_->lastUpwardDb     = 0.0f;
    impl_->lastDownwardDb   = 0.0f;
}

void DynamicsProcessor::updateParameters(const DynamicsParameters& params) noexcept
{
    impl_->params = params;
    impl_->recalculateParameters();
}

const DynamicsParameters& DynamicsProcessor::getParameters() const noexcept
{
    return impl_->params;
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
        impl_->lastGainDb     = 0.0f;
        impl_->lastUpwardDb   = 0.0f;
        impl_->lastDownwardDb = 0.0f;
        return;
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxSupportedChannels)));
    const bool useHp = impl_->params.useAdvancedMode ? (impl_->params.scLowHz > 25.0f) : (impl_->params.scLowNorm > 0.001f);
    const bool useLp = impl_->params.useAdvancedMode ? (impl_->params.scHighHz < 19000.0f) : (impl_->params.scHighNorm < 0.999f);

    float blockLufs     = impl_->lastLufs;
    float blockGainDb   = impl_->lastGainDb;
    float blockPeakLin  = 0.0f;
    float blockUpward   = 0.0f;
    float blockDownward = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float weightedSquareSum = 0.0f;

        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] == nullptr)
                continue;

            const float x = channelPointers[ch][i];
            const float weight = getBs1770ChannelWeight(ch, activeCh);

            const float absX = std::abs(x);
            if (absX > blockPeakLin)
                blockPeakLin = absX;

            if (weight <= 0.0f)
                continue; // LFE u otros canales no ponderados

            float sc = impl_->processKWeightedSample(ch, x);
            if (useHp)
                sc = impl_->scHighPass[ch].processSample(0, sc);
            if (useLp)
                sc = impl_->scLowPass[ch].processSample(0, sc);

            weightedSquareSum += weight * (sc * sc);
        }

        const float detCoeff = (weightedSquareSum > impl_->meanSquareEnv) ? impl_->detAtkCoeff : impl_->detRelCoeff;
        impl_->meanSquareEnv = detCoeff * impl_->meanSquareEnv + (1.0f - detCoeff) * weightedSquareSum;

        const float currentLufs = -0.691f + 10.0f * std::log10(juce::jmax(impl_->meanSquareEnv, 1.0e-12f));
        blockLufs = currentLufs;

        float curUpDb = 0.0f;
        float curDownDb = 0.0f;
        const float targetGainDb = impl_->computeHybridTargetGainDb(currentLufs, curUpDb, curDownDb);

        const float gainCoeff = (targetGainDb < impl_->smoothedGainDb) ? impl_->gainAtkCoeff : impl_->gainRelCoeff;
        impl_->smoothedGainDb = gainCoeff * impl_->smoothedGainDb + (1.0f - gainCoeff) * targetGainDb;
        blockGainDb   = impl_->smoothedGainDb;
        blockUpward   = curUpDb;
        blockDownward = curDownDb;

        const float compGainLin = juce::Decibels::decibelsToGain(impl_->smoothedGainDb);

        // Limitador de seguridad de muestra exacta post-compresión (-0.5 dBFS)
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

    impl_->lastLufs       = blockLufs;
    impl_->lastGainDb     = blockGainDb + impl_->peakLimiter.getCurrentReductionDb();
    impl_->lastPeakDb     = juce::Decibels::gainToDecibels(blockPeakLin, -100.0f);
    impl_->lastUpwardDb   = blockUpward;
    impl_->lastDownwardDb = blockDownward;
}

float DynamicsProcessor::getCurrentLufs() const noexcept
{
    return impl_->lastLufs;
}

float DynamicsProcessor::getCurrentGainDb() const noexcept
{
    return impl_->lastGainDb;
}

float DynamicsProcessor::getCurrentPeakDb() const noexcept
{
    return impl_->lastPeakDb;
}

float DynamicsProcessor::getUpwardGainDb() const noexcept
{
    return impl_->lastUpwardDb;
}

float DynamicsProcessor::getDownwardGainDb() const noexcept
{
    return impl_->lastDownwardDb;
}

} // namespace dash::core
