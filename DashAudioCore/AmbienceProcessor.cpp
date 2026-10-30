#include "AmbienceProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace dash::core
{

float AmbienceProcessor::mapScNormToHz(float norm) noexcept
{
    // Mapeo logarítmico canónico: norm=0.2269 -> 350 Hz; norm=0.5293 -> 2500 Hz
    return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, norm));
}

struct AmbienceProcessor::Impl
{
    double sampleRate = 48000.0;
    AmbienceParameters params {};

    std::array<juce::dsp::LinkwitzRileyFilter<float>, kMaxChannels> lowMidCrossovers;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, kMaxChannels> midHighCrossovers;

    std::array<float, kMaxChannels> envShort {};
    std::array<float, kMaxChannels> envLong  {};

    std::array<float, kMaxChannels> smoothedGainLowDb  {};
    std::array<float, kMaxChannels> smoothedGainMidDb  {};
    std::array<float, kMaxChannels> smoothedGainHighDb {};

    float shortAtkCoeff = 0.9f;
    float shortRelCoeff = 0.99f;
    float longAtkCoeff  = 0.999f;
    float longRelCoeff  = 0.9998f;

    float gainAtkCoeff = 0.99f;
    float gainRelCoeff = 0.999f;

    float currentHpHz = 350.0f;
    float currentLpHz = 2500.0f;
    float lastGainDb  = 0.0f;
    float stationarityIndex = 0.0f;

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    void recalculateParameters() noexcept
    {
        currentHpHz = mapScNormToHz(params.scLow);
        currentLpHz = mapScNormToHz(params.scHigh);

        const float nyquistGuard = static_cast<float>(sampleRate * 0.48);
        const float safeLowHz    = juce::jlimit(40.0f, nyquistGuard - 200.0f, currentHpHz);
        const float safeHighHz   = juce::jlimit(safeLowHz + 100.0f, nyquistGuard, currentLpHz);

        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            lowMidCrossovers[ch].setCutoffFrequency(safeLowHz);
            midHighCrossovers[ch].setCutoffFrequency(safeHighHz);
        }

        // Tiempos del detector de estacionariedad
        shortAtkCoeff = calcOnePoleCoeff(0.005f); // 5 ms corto plazo
        shortRelCoeff = calcOnePoleCoeff(0.040f); // 40 ms corto plazo
        longAtkCoeff  = calcOnePoleCoeff(0.120f); // 120 ms largo plazo
        longRelCoeff  = calcOnePoleCoeff(0.600f); // 600 ms largo plazo

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

    /**
     * Calcula el factor de estacionariedad (0.0 = transitorio activo; 1.0 = ruido estacionario).
     */
    float estimateStationarity(size_t ch, float sample) noexcept
    {
        const float absX = std::abs(sample);

        const float sCoeff = (absX > envShort[ch]) ? shortAtkCoeff : shortRelCoeff;
        envShort[ch] = sCoeff * envShort[ch] + (1.0f - sCoeff) * absX;

        const float lCoeff = (absX > envLong[ch]) ? longAtkCoeff : longRelCoeff;
        envLong[ch] = lCoeff * envLong[ch] + (1.0f - lCoeff) * absX;

        // Si la energía a corto y largo plazo difiere notablemente (flujo dinámico alto),
        // se clasifica como sonido útil o transitorio (paso, voz, disparo).
        const float diff = std::abs(envShort[ch] - envLong[ch]);
        const float denom = envLong[ch] + 1.0e-4f;
        const float dynamicFlux = diff / denom;

        const float sens = params.useAdvancedMode ? params.stationaritySensitivity : (params.distant ? 0.65f : 0.40f);
        const float fluxThreshold = 0.35f + (1.0f - sens) * 0.45f;

        if (dynamicFlux > fluxThreshold)
            return 0.0f; // Sonido dinámico útil: protección total contra atenuación

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

    for (size_t ch = 0; ch < kMaxChannels; ++ch)
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
    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        impl_->lowMidCrossovers[ch].reset();
        impl_->midHighCrossovers[ch].reset();
        impl_->envShort[ch]             = 0.0f;
        impl_->envLong[ch]              = 0.0f;
        impl_->smoothedGainLowDb[ch]    = 0.0f;
        impl_->smoothedGainMidDb[ch]    = 0.0f;
        impl_->smoothedGainHighDb[ch]   = 0.0f;
    }
    impl_->lastGainDb        = 0.0f;
    impl_->stationarityIndex = 0.0f;
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

    if (impl_->params.bypass)
    {
        impl_->lastGainDb        = 0.0f;
        impl_->stationarityIndex = 0.0f;
        return;
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));
    float blockMaxReductionDb = 0.0f;
    float blockStationaritySum = 0.0f;

    const float ambGuarNorm = juce::jlimit(0.0f, 1.0f, impl_->params.ambGuar);
    if (ambGuarNorm <= 0.0001f)
        return; // Sin atenuación

    // Parámetros de profundidad por banda
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

            // 1. Clasificación acústica de estacionariedad espectral
            const float stat = impl_->estimateStationarity(ch, input);
            blockStationaritySum += stat;

            // 2. División Linkwitz-Riley en 3 bandas alineadas en fase
            float bandLow = 0.0f;
            float restHigh = 0.0f;
            impl_->lowMidCrossovers[ch].processSample(0, input, bandLow, restHigh);

            float bandMid = 0.0f;
            float bandHigh = 0.0f;
            impl_->midHighCrossovers[ch].processSample(0, restHigh, bandMid, bandHigh);

            // 3. Cálculo de ganancia atenuadora por banda
            const float targetLowDb  = (impl_->params.useAdvancedMode && impl_->params.bypassLowBand)  ? 0.0f : (lowMaxDb * stat);
            const float targetMidDb  = (impl_->params.useAdvancedMode && impl_->params.bypassMidBand)  ? 0.0f : (midMaxDb * stat);
            const float targetHighDb = (impl_->params.useAdvancedMode && impl_->params.bypassHighBand) ? 0.0f : (highMaxDb * stat);

            // Suavizado temporal continuo para evitar chasquidos
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

            // 4. Recombinación de las 3 bandas espectrales
            channelPointers[ch][i] = (bandLow * gLowLin) + (bandMid * gMidLin) + (bandHigh * gHighLin);
        }
    }

    impl_->lastGainDb        = blockMaxReductionDb;
    impl_->stationarityIndex = blockStationaritySum / static_cast<float>(numSamples * activeCh);
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
    return impl_->lastGainDb;
}

float AmbienceProcessor::getStationarityIndex() const noexcept
{
    return impl_->stationarityIndex;
}

} // namespace dash::core
