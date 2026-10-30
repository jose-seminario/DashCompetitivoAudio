#include "AmbienceProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace dash::core
{

float AmbienceProcessor::mapScNormToHz(float norm) noexcept
{
    const float x = std::clamp(norm, 0.0f, 1.0f);

    if (x <= 0.2269f)
    {
        const float t = x / 0.2269f;
        return 20.0f * std::pow(350.0f / 20.0f, t);
    }

    if (x <= 0.5293f)
    {
        const float t = (x - 0.2269f) / (0.5293f - 0.2269f);
        return 350.0f * std::pow(2500.0f / 350.0f, t);
    }

    const float t = (x - 0.5293f) / (1.0f - 0.5293f);
    return 2500.0f * std::pow(20000.0f / 2500.0f, t);
}

struct AmbienceProcessor::Impl
{
    double sampleRate = 48000.0;
    AmbienceParameters params {};

    juce::dsp::IIR::Coefficients<float>::Ptr sharedScHpCoeffs;
    juce::dsp::IIR::Coefficients<float>::Ptr sharedScLpCoeffs;
    std::array<juce::dsp::IIR::Filter<float>, kMaxChannels> scHighPass;
    std::array<juce::dsp::IIR::Filter<float>, kMaxChannels> scLowPass;

    juce::dsp::LinkwitzRileyFilter<float> crossoverLowMid;
    juce::dsp::LinkwitzRileyFilter<float> crossoverMidHigh;
    juce::dsp::LinkwitzRileyFilter<float> lowBandPhaseAlign;

    float cachedScLow  = -1.0f;
    float cachedScHigh = -1.0f;
    float currentHpHz  = 350.0f;
    float currentLpHz  = 2500.0f;

    float detAtkCoeff  = 0.99f;
    float detRelCoeff  = 0.999f;
    float ambRiseCoeff = 0.9999f;
    float ambFallCoeff = 0.999f;
    float gainAtkCoeff = 0.99f;
    float gainRelCoeff = 0.999f;
    float thresholdDb  = -24.0f;

    float scMeanSquare = 1.0e-9f;
    std::array<float, 3> bandRmsSq      { 1.0e-9f, 1.0e-9f, 1.0e-9f };
    std::array<float, 3> ambFloorSq     { 1.0e-9f, 1.0e-9f, 1.0e-9f };
    std::array<float, 3> smoothedGainDb { 0.0f, 0.0f, 0.0f };
    float lastMidGainDb = 0.0f;

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    [[nodiscard]] float computeBandDownwardGainDb(
        float levelDb,
        float stationarity,
        bool isTacticalMidBand) const noexcept
    {
        const float guardDepth = juce::jlimit(0.0f, 1.0f, params.ambGuar);
        float totalReductionDb = 0.0f;

        if (levelDb > -52.0f && guardDepth > 0.001f)
        {
            const float floorExcessNorm = juce::jlimit(0.0f, 1.0f, (levelDb - (-52.0f)) / 24.0f);
            float effectiveStationarity = stationarity * stationarity;

            if (isTacticalMidBand && params.distant)
                effectiveStationarity *= 0.45f;

            const float maxAmbientDuckDb = isTacticalMidBand ? -3.5f : -6.0f;
            totalReductionDb += maxAmbientDuckDb * guardDepth * floorExcessNorm * effectiveStationarity;
        }

        constexpr float kKneeWidthDb   = 6.0f;
        constexpr float kDownwardSlope = 0.50f;

        const float overDb = levelDb - thresholdDb;
        if (overDb > -0.5f * kKneeWidthDb)
        {
            float compressedOverDb = 0.0f;
            if (overDb < 0.5f * kKneeWidthDb)
            {
                const float x = overDb + 0.5f * kKneeWidthDb;
                compressedOverDb = (x * x) / (2.0f * kKneeWidthDb);
            }
            else
            {
                compressedOverDb = overDb;
            }

            const float bandSlopeScale = (isTacticalMidBand && params.distant) ? 0.70f : 1.0f;
            totalReductionDb -= compressedOverDb * kDownwardSlope * bandSlopeScale * juce::jmax(0.35f, guardDepth);
        }

        return juce::jlimit(-15.0f, 0.0f, totalReductionDb);
    }

    void recalculateParameters() noexcept
    {
        if (std::abs(params.scLow - cachedScLow) > 1.0e-5f
            || std::abs(params.scHigh - cachedScHigh) > 1.0e-5f)
        {
            cachedScLow  = params.scLow;
            cachedScHigh = params.scHigh;

            const float nyquistLimit = static_cast<float>(sampleRate * 0.47);
            const float hpHz = juce::jlimit(20.0f, nyquistLimit - 200.0f, mapScNormToHz(cachedScLow));
            const float lpHz = juce::jlimit(hpHz + 120.0f, nyquistLimit, mapScNormToHz(cachedScHigh));

            currentHpHz = hpHz;
            currentLpHz = lpHz;

            if (sharedScHpCoeffs != nullptr)
            {
                *sharedScHpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
                    sampleRate, hpHz, 0.70710678f);
            }

            if (sharedScLpCoeffs != nullptr)
            {
                *sharedScLpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
                    sampleRate, lpHz, 0.70710678f);
            }

            crossoverLowMid.setCutoffFrequency(hpHz);
            crossoverMidHigh.setCutoffFrequency(lpHz);
            lowBandPhaseAlign.setCutoffFrequency(lpHz);
        }

        detAtkCoeff = calcOnePoleCoeff(0.012f);
        const float detRelSec = 0.040f + juce::jlimit(0.0f, 1.0f, params.detRel) * 0.556f;
        detRelCoeff = calcOnePoleCoeff(detRelSec);

        ambRiseCoeff = calcOnePoleCoeff(1.500f);
        ambFallCoeff = calcOnePoleCoeff(0.110f);

        gainAtkCoeff = calcOnePoleCoeff(0.008f);
        const float relSec = 0.035f + juce::jlimit(0.0f, 1.0f, params.release) * 0.385f;
        gainRelCoeff = calcOnePoleCoeff(relSec);

        thresholdDb = -38.0f + juce::jlimit(0.0f, 1.0f, params.target) * 36.0f;
    }
};

AmbienceProcessor::AmbienceProcessor()
    : impl_(std::make_unique<Impl>())
{
}

AmbienceProcessor::~AmbienceProcessor() = default;
AmbienceProcessor::AmbienceProcessor(AmbienceProcessor&&) noexcept = default;
AmbienceProcessor& AmbienceProcessor::operator=(AmbienceProcessor&&) noexcept = default;

void AmbienceProcessor::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    (void)numChannels;
    impl_->sampleRate = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    const auto safeBlock = static_cast<juce::uint32>(std::max(1, maximumBlockSize));

    impl_->sharedScHpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        impl_->sampleRate, 350.0f, 0.70710678f);
    impl_->sharedScLpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        impl_->sampleRate, 2500.0f, 0.70710678f);

    juce::dsp::ProcessSpec monoSpec {};
    monoSpec.sampleRate       = impl_->sampleRate;
    monoSpec.maximumBlockSize = safeBlock;
    monoSpec.numChannels      = 1;

    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        impl_->scHighPass[ch].prepare(monoSpec);
        impl_->scHighPass[ch].coefficients = impl_->sharedScHpCoeffs;

        impl_->scLowPass[ch].prepare(monoSpec);
        impl_->scLowPass[ch].coefficients = impl_->sharedScLpCoeffs;
    }

    juce::dsp::ProcessSpec multiSpec {};
    multiSpec.sampleRate       = impl_->sampleRate;
    multiSpec.maximumBlockSize = safeBlock;
    multiSpec.numChannels      = static_cast<juce::uint32>(kMaxChannels);

    impl_->crossoverLowMid.prepare(multiSpec);
    impl_->crossoverLowMid.setCutoffFrequency(350.0f);

    impl_->crossoverMidHigh.prepare(multiSpec);
    impl_->crossoverMidHigh.setCutoffFrequency(2500.0f);

    impl_->lowBandPhaseAlign.prepare(multiSpec);
    impl_->lowBandPhaseAlign.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
    impl_->lowBandPhaseAlign.setCutoffFrequency(2500.0f);

    impl_->cachedScLow  = 0.2269f;
    impl_->cachedScHigh = 0.5293f;
    impl_->currentHpHz  = 350.0f;
    impl_->currentLpHz  = 2500.0f;

    impl_->recalculateParameters();
    reset();
}

void AmbienceProcessor::reset() noexcept
{
    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        impl_->scHighPass[ch].reset();
        impl_->scLowPass[ch].reset();
    }

    impl_->crossoverLowMid.reset();
    impl_->crossoverMidHigh.reset();
    impl_->lowBandPhaseAlign.reset();

    impl_->scMeanSquare   = 1.0e-9f;
    impl_->bandRmsSq      = { 1.0e-9f, 1.0e-9f, 1.0e-9f };
    impl_->ambFloorSq     = { 1.0e-9f, 1.0e-9f, 1.0e-9f };
    impl_->smoothedGainDb = { 0.0f, 0.0f, 0.0f };
    impl_->lastMidGainDb  = 0.0f;
}

void AmbienceProcessor::updateParameters(const AmbienceParameters& params) noexcept
{
    impl_->params = params;
    impl_->recalculateParameters();
}

void AmbienceProcessor::process(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    float* channels[1] = { buffer };
    process(channels, 1, numSamples);
}

void AmbienceProcessor::process(std::vector<float>& buffer) noexcept
{
    if (buffer.empty())
        return;

    process(buffer.data(), static_cast<int>(buffer.size()));
}

void AmbienceProcessor::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    if (impl_->params.bypass)
    {
        impl_->lastMidGainDb = 0.0f;
        return;
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));
    const float invCh     = 1.0f / static_cast<float>(activeCh);

    std::array<float, kMaxChannels> lowSamples {};
    std::array<float, kMaxChannels> midSamples {};
    std::array<float, kMaxChannels> highSamples {};

    for (int i = 0; i < numSamples; ++i)
    {
        float scEnergySum   = 0.0f;
        float lowEnergySum  = 0.0f;
        float highEnergySum = 0.0f;

        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] == nullptr)
                continue;

            const float x = channelPointers[ch][i];

            const float scHp = impl_->scHighPass[ch].processSample(x);
            const float scBp = impl_->scLowPass[ch].processSample(scHp);
            scEnergySum += scBp * scBp;

            if (impl_->params.bandMod)
            {
                float lowRaw  = 0.0f;
                float midHigh = 0.0f;
                impl_->crossoverLowMid.processSample(static_cast<int>(ch), x, lowRaw, midHigh);

                const float lowAligned = impl_->lowBandPhaseAlign.processSample(static_cast<int>(ch), lowRaw);

                float mid  = 0.0f;
                float high = 0.0f;
                impl_->crossoverMidHigh.processSample(static_cast<int>(ch), midHigh, mid, high);

                lowSamples[ch]  = lowAligned;
                midSamples[ch]  = mid;
                highSamples[ch] = high;

                lowEnergySum  += lowAligned * lowAligned;
                highEnergySum += high * high;
            }
        }

        const float scMeanSq = scEnergySum * invCh;
        const float scCoeff  = (scMeanSq > impl_->scMeanSquare) ? impl_->detAtkCoeff : impl_->detRelCoeff;
        impl_->scMeanSquare  = scCoeff * impl_->scMeanSquare + (1.0f - scCoeff) * scMeanSq;
        const float scDb     = 10.0f * std::log10(juce::jmax(impl_->scMeanSquare, 1.0e-12f));

        if (impl_->params.bandMod)
        {
            const std::array<float, 3> instSq {
                lowEnergySum * invCh,
                scMeanSq,
                highEnergySum * invCh
            };

            for (size_t b = 0; b < 3; ++b)
            {
                const float dCoeff = (instSq[b] > impl_->bandRmsSq[b]) ? impl_->detAtkCoeff : impl_->detRelCoeff;
                impl_->bandRmsSq[b] = dCoeff * impl_->bandRmsSq[b] + (1.0f - dCoeff) * instSq[b];

                const float aCoeff = (impl_->bandRmsSq[b] > impl_->ambFloorSq[b])
                    ? impl_->ambRiseCoeff
                    : impl_->ambFallCoeff;
                impl_->ambFloorSq[b] = aCoeff * impl_->ambFloorSq[b] + (1.0f - aCoeff) * impl_->bandRmsSq[b];

                const float bandDb = (b == 1)
                    ? scDb
                    : 10.0f * std::log10(juce::jmax(impl_->bandRmsSq[b], 1.0e-12f));

                const float stationarity = juce::jlimit(
                    0.0f, 1.0f, impl_->ambFloorSq[b] / (impl_->bandRmsSq[b] + 1.0e-11f));
                const bool isTacticalMidBand = (b == 1);

                const float targetGainDb = impl_->computeBandDownwardGainDb(bandDb, stationarity, isTacticalMidBand);
                const float gCoeff = (targetGainDb < impl_->smoothedGainDb[b])
                    ? impl_->gainAtkCoeff
                    : impl_->gainRelCoeff;
                impl_->smoothedGainDb[b] = gCoeff * impl_->smoothedGainDb[b] + (1.0f - gCoeff) * targetGainDb;
            }

            const float gLow  = juce::Decibels::decibelsToGain(impl_->smoothedGainDb[0]);
            const float gMid  = juce::Decibels::decibelsToGain(impl_->smoothedGainDb[1]);
            const float gHigh = juce::Decibels::decibelsToGain(impl_->smoothedGainDb[2]);

            for (size_t ch = 0; ch < activeCh; ++ch)
            {
                if (channelPointers[ch] != nullptr)
                    channelPointers[ch][i] = lowSamples[ch] * gLow + midSamples[ch] * gMid + highSamples[ch] * gHigh;
            }
        }
        else
        {
            const float aCoeff = (impl_->scMeanSquare > impl_->ambFloorSq[1])
                ? impl_->ambRiseCoeff
                : impl_->ambFallCoeff;
            impl_->ambFloorSq[1] = aCoeff * impl_->ambFloorSq[1] + (1.0f - aCoeff) * impl_->scMeanSquare;

            const float stationarity = juce::jlimit(
                0.0f, 1.0f, impl_->ambFloorSq[1] / (impl_->scMeanSquare + 1.0e-11f));
            const float targetGainDb = impl_->computeBandDownwardGainDb(scDb, stationarity, true);

            const float gCoeff = (targetGainDb < impl_->smoothedGainDb[1])
                ? impl_->gainAtkCoeff
                : impl_->gainRelCoeff;
            impl_->smoothedGainDb[1] = gCoeff * impl_->smoothedGainDb[1] + (1.0f - gCoeff) * targetGainDb;

            const float gLin = juce::Decibels::decibelsToGain(impl_->smoothedGainDb[1]);
            for (size_t ch = 0; ch < activeCh; ++ch)
            {
                if (channelPointers[ch] != nullptr)
                    channelPointers[ch][i] *= gLin;
            }
        }
    }

    impl_->crossoverLowMid.snapToZero();
    impl_->crossoverMidHigh.snapToZero();
    impl_->lowBandPhaseAlign.snapToZero();

    impl_->lastMidGainDb = impl_->smoothedGainDb[1];
}

float AmbienceProcessor::getCurrentHpHz() const noexcept
{
    return impl_->currentHpHz;
}

float AmbienceProcessor::getCurrentLpHz() const noexcept
{
    return impl_->currentLpHz;
}

float AmbienceProcessor::getCurrentGainDb() const noexcept
{
    return impl_->lastMidGainDb;
}

} // namespace dash::core
