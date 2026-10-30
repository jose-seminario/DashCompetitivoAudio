#include "AmbienceProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

namespace dash::core
{

float AmbienceProcessor::mapScNormToHz(float norm) noexcept
{
    return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, norm));
}

struct AmbienceProcessor::Impl
{
    double sampleRate = 48000.0;
    AmbienceParameters params {};

    std::array<juce::dsp::LinkwitzRileyFilter<float>, kMaxSupportedChannels> lowMidCrossovers;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, kMaxSupportedChannels> midHighCrossovers;

    std::array<float, kMaxSupportedChannels> envShort {};
    std::array<float, kMaxSupportedChannels> envLong  {};

    std::array<float, kMaxSupportedChannels> smoothedGainLowDb  {};
    std::array<float, kMaxSupportedChannels> smoothedGainMidDb  {};
    std::array<float, kMaxSupportedChannels> smoothedGainHighDb {};

    float shortCoeff = 0.99f;
    float longCoeff  = 0.999f;

    float gainAtkCoeff = 0.99f;
    float gainRelCoeff = 0.999f;

    std::atomic<float> currentHpHz       { 350.0f };
    std::atomic<float> currentLpHz       { 2500.0f };
    std::atomic<float> lastGainDb        { 0.0f };
    std::atomic<float> stationarityIndex { 0.0f };

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    void recalculateParameters() noexcept
    {
        const float hpHz = mapScNormToHz(params.scLow);
        const float lpHz = mapScNormToHz(params.scHigh);
        currentHpHz.store(hpHz, std::memory_order_relaxed);
        currentLpHz.store(lpHz, std::memory_order_relaxed);

        const float nyquistGuard = static_cast<float>(sampleRate * 0.48);
        const float safeLowHz    = juce::jlimit(40.0f, nyquistGuard - 200.0f, hpHz);
        const float safeHighHz   = juce::jlimit(safeLowHz + 100.0f, nyquistGuard, lpHz);

        for (size_t ch = 0; ch < kMaxSupportedChannels; ++ch)
        {
            lowMidCrossovers[ch].setCutoffFrequency(safeLowHz);
            midHighCrossovers[ch].setCutoffFrequency(safeHighHz);
        }

        // Seguidores de envolvente simétricos de media absoluta
        shortCoeff = calcOnePoleCoeff(0.015f);
        longCoeff  = calcOnePoleCoeff(0.080f);

        if (params.useAdvancedMode)
        {
            gainAtkCoeff = calcOnePoleCoeff(juce::jmax(0.001f, params.attackMs * 0.001f));
            gainRelCoeff = calcOnePoleCoeff(juce::jmax(0.010f, params.releaseMs * 0.001f));
        }
        else
        {
            gainAtkCoeff = calcOnePoleCoeff(0.020f);
            const float relSec = 0.050f + juce::jlimit(0.0f, 1.0f, params.release) * 0.450f;
            gainRelCoeff = calcOnePoleCoeff(relSec);
        }
    }

    float estimateStationarity(size_t ch, float sample) noexcept
    {
        const float absX = std::abs(sample);

        // Integración simétrica de energía rectificada
        envShort[ch] = shortCoeff * envShort[ch] + (1.0f - shortCoeff) * absX;
        envLong[ch]  = longCoeff  * envLong[ch]  + (1.0f - longCoeff)  * absX;

        // Compuerta acústica de silencio: el silencio absoluto nunca debe ser clasificado como ruido
        if (envLong[ch] < 0.002f && envShort[ch] < 0.002f)
            return 0.0f;

        const float diff = std::abs(envShort[ch] - envLong[ch]);
        const float denom = envLong[ch] + 1.0e-4f;
        const float dynamicFlux = diff / denom;

        const float sens = params.useAdvancedMode ? params.stationaritySensitivity : (params.distant ? 0.65f : 0.40f);
        const float fluxThreshold = 0.35f + (1.0f - sens) * 0.45f;

        if (dynamicFlux > fluxThreshold)
            return 0.0f;

        const float rawStat = 1.0f - (dynamicFlux / fluxThreshold);
        return juce::jlimit(0.0f, 1.0f, rawStat * rawStat);
    }
};

AmbienceProcessor::AmbienceProcessor()
    : impl_(std::make_unique<Impl>())
{
}

AmbienceProcessor::~AmbienceProcessor() = default;
AmbienceProcessor::AmbienceProcessor(AmbienceProcessor&&) noexcept = default;
AmbienceProcessor& AmbienceProcessor::operator=(AmbienceProcessor&&) noexcept = default;

void AmbienceProcessor::prepare(double sampleRate, int maximumBlockSize, int /*numChannels*/)
{
    const double safeSr = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    const int safeBlock = std::max(1, maximumBlockSize);

    impl_->sampleRate = safeSr;

    juce::dsp::ProcessSpec monoSpec {};
    monoSpec.sampleRate       = safeSr;
    monoSpec.maximumBlockSize = static_cast<juce::uint32>(safeBlock);
    monoSpec.numChannels      = 1;

    for (size_t ch = 0; ch < kMaxSupportedChannels; ++ch)
    {
        impl_->lowMidCrossovers[ch].prepare(monoSpec);
        impl_->lowMidCrossovers[ch].setType(juce::dsp::LinkwitzRileyFilterType::allpass);

        impl_->midHighCrossovers[ch].prepare(monoSpec);
        impl_->midHighCrossovers[ch].setType(juce::dsp::LinkwitzRileyFilterType::allpass);
    }

    impl_->recalculateParameters();
    reset();
}

void AmbienceProcessor::reset() noexcept
{
    for (size_t ch = 0; ch < kMaxSupportedChannels; ++ch)
    {
        impl_->lowMidCrossovers[ch].reset();
        impl_->midHighCrossovers[ch].reset();
        impl_->envShort[ch]             = 0.0f;
        impl_->envLong[ch]              = 0.0f;
        impl_->smoothedGainLowDb[ch]    = 0.0f;
        impl_->smoothedGainMidDb[ch]    = 0.0f;
        impl_->smoothedGainHighDb[ch]   = 0.0f;
    }
    impl_->lastGainDb.store(0.0f, std::memory_order_relaxed);
    impl_->stationarityIndex.store(0.0f, std::memory_order_relaxed);
}

void AmbienceProcessor::updateParameters(const AmbienceParameters& params) noexcept
{
    impl_->params = params;
    impl_->recalculateParameters();
}

const AmbienceParameters& AmbienceProcessor::getParameters() const noexcept
{
    return impl_->params;
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

    juce::ScopedNoDenormals noDenormals;

    if (impl_->params.bypass)
    {
        impl_->lastGainDb.store(0.0f, std::memory_order_relaxed);
        impl_->stationarityIndex.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxSupportedChannels)));
    float blockMaxReductionDb = 0.0f;
    float blockStationaritySum = 0.0f;

    const float ambGuarNorm = juce::jlimit(0.0f, 1.0f, impl_->params.ambGuar);
    if (ambGuarNorm <= 0.0001f)
    {
        impl_->lastGainDb.store(0.0f, std::memory_order_relaxed);
        impl_->stationarityIndex.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float lowMaxDb  = impl_->params.useAdvancedMode ? impl_->params.lowReductionDepthDb  : (-4.0f * ambGuarNorm);
    const float midMaxDb  = impl_->params.useAdvancedMode ? impl_->params.midReductionDepthDb  : (-6.5f * ambGuarNorm);
    const float highMaxDb = impl_->params.useAdvancedMode ? impl_->params.highReductionDepthDb : (-3.5f * ambGuarNorm);

    for (int i = 0; i < numSamples; ++i)
    {
        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] == nullptr)
                continue;

            const float input = channelPointers[ch][i];

            const float stat = impl_->estimateStationarity(ch, input);
            blockStationaritySum += stat;

            float bandLow = 0.0f;
            float restHigh = 0.0f;
            impl_->lowMidCrossovers[ch].processSample(0, input, bandLow, restHigh);

            float bandMid = 0.0f;
            float bandHigh = 0.0f;
            impl_->midHighCrossovers[ch].processSample(0, restHigh, bandMid, bandHigh);

            const float targetLowDb  = impl_->params.bypassLowBand  ? 0.0f : (lowMaxDb * stat);
            const float targetMidDb  = impl_->params.bypassMidBand  ? 0.0f : (midMaxDb * stat);
            const float targetHighDb = impl_->params.bypassHighBand ? 0.0f : (highMaxDb * stat);

            const auto smoothGain = [this](float target, float& smoothed) -> float {
                const float coeff = (target < smoothed) ? impl_->gainAtkCoeff : impl_->gainRelCoeff;
                smoothed = coeff * smoothed + (1.0f - coeff) * target;
                return smoothed;
            };

            const float gLowDb  = smoothGain(targetLowDb,  impl_->smoothedGainLowDb[ch]);
            const float gMidDb  = smoothGain(targetMidDb,  impl_->smoothedGainMidDb[ch]);
            const float gHighDb = smoothGain(targetHighDb, impl_->smoothedGainHighDb[ch]);

            if (gMidDb < blockMaxReductionDb)
                blockMaxReductionDb = gMidDb;

            const float gLowLin  = juce::Decibels::decibelsToGain(gLowDb);
            const float gMidLin  = juce::Decibels::decibelsToGain(gMidDb);
            const float gHighLin = juce::Decibels::decibelsToGain(gHighDb);

            channelPointers[ch][i] = (bandLow * gLowLin) + (bandMid * gMidLin) + (bandHigh * gHighLin);
        }
    }

    impl_->lastGainDb.store(blockMaxReductionDb, std::memory_order_relaxed);
    impl_->stationarityIndex.store(blockStationaritySum / static_cast<float>(numSamples * activeCh), std::memory_order_relaxed);
}

float AmbienceProcessor::getCurrentHpHz() const noexcept
{
    return impl_->currentHpHz.load(std::memory_order_relaxed);
}

float AmbienceProcessor::getCurrentLpHz() const noexcept
{
    return impl_->currentLpHz.load(std::memory_order_relaxed);
}

float AmbienceProcessor::getCurrentGainDb() const noexcept
{
    return impl_->lastGainDb.load(std::memory_order_relaxed);
}

float AmbienceProcessor::getStationarityIndex() const noexcept
{
    return impl_->stationarityIndex.load(std::memory_order_relaxed);
}

} // namespace dash::core
