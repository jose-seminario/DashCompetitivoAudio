#include "TransientProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

namespace dash::core
{

struct TransientProcessor::Impl
{
    double sampleRate       = 48000.0;
    int    centerChannelIdx = 2;
    TransientParameters params {};

    std::array<juce::dsp::LinkwitzRileyFilter<float>, static_cast<size_t>(kMaxChannels)> crossovers;

    std::array<float, static_cast<size_t>(kMaxChannels)> envFast {};
    std::array<float, static_cast<size_t>(kMaxChannels)> envSlow {};
    std::array<float, static_cast<size_t>(kMaxChannels)> smoothedGainDb {};

    float fastAtkCoeff = 0.9f;
    float fastRelCoeff = 0.999f;
    float slowAtkCoeff = 0.99f;
    float slowRelCoeff = 0.9995f;
    float gainAtkCoeff = 0.9f;
    float gainRelCoeff = 0.999f;

    std::atomic<float> activeSplitHz    { kDefaultSplitHz };
    float maxAttenuationDb              = -6.93575f;
    std::atomic<float> lastReductionDb  { 0.0f };

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.00001f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    void recalculateParameters() noexcept
    {
        const float split = juce::jlimit(kMinSplitHz, kMaxSplitHz,
                                         params.useAdvancedMode ? params.frequencySplitHz : kDefaultSplitHz);
        activeSplitHz.store(split, std::memory_order_relaxed);

        for (size_t ch = 0; ch < static_cast<size_t>(kMaxChannels); ++ch)
            crossovers[ch].setCutoffFrequency(split);

        if (params.useAdvancedMode)
        {
            fastAtkCoeff = calcOnePoleCoeff(juce::jmax(0.00005f, params.attackMs * 0.001f));
            fastRelCoeff = calcOnePoleCoeff(juce::jmax(0.005f, params.releaseMs * 0.001f));
            slowAtkCoeff = calcOnePoleCoeff(0.020f);
            slowRelCoeff = calcOnePoleCoeff(0.240f);
            gainAtkCoeff = calcOnePoleCoeff(juce::jmax(0.0001f, params.attackMs * 0.001f));
            gainRelCoeff = calcOnePoleCoeff(juce::jmax(0.005f, params.releaseMs * 0.001f));
            maxAttenuationDb = juce::jlimit(-30.0f, 0.0f, params.maxAttenuationDb);
        }
        else
        {
            // Ataque ultrarrápido para suprimir el primer impulso del disparo (muestra exacta)
            fastAtkCoeff = calcOnePoleCoeff(0.00002f);
            const float relSec = 0.020f + juce::jlimit(0.0f, 1.0f, params.release) * 0.160f;
            fastRelCoeff = calcOnePoleCoeff(relSec);

            slowAtkCoeff = calcOnePoleCoeff(0.018f);
            slowRelCoeff = calcOnePoleCoeff(0.220f);

            gainAtkCoeff = calcOnePoleCoeff(0.00003f);
            gainRelCoeff = calcOnePoleCoeff(relSec);

            const float clampedGun = juce::jlimit(0.10f, 1.0f, params.gun);
            maxAttenuationDb = juce::Decibels::gainToDecibels(clampedGun, -24.0f);
        }
    }

    float evaluateTransientSample(size_t ch, float crackSample, float externalMaxDb = 0.0f) noexcept
    {
        const float absCrack = std::abs(crackSample);

        const float fCoeff = (absCrack > envFast[ch]) ? fastAtkCoeff : fastRelCoeff;
        envFast[ch] = fCoeff * envFast[ch] + (1.0f - fCoeff) * absCrack;

        const float sCoeff = (absCrack > envSlow[ch]) ? slowAtkCoeff : slowRelCoeff;
        envSlow[ch] = sCoeff * envSlow[ch] + (1.0f - sCoeff) * absCrack;

        float minPeakThreshold   = 0.045f;
        float requiredCrestRatio = 1.70f;

        if (params.useAdvancedMode)
        {
            const float sens = juce::jlimit(0.0f, 1.0f, params.sensitivity);
            minPeakThreshold   = 0.080f - (sens * 0.055f);
            requiredCrestRatio = 2.40f - (sens * 1.00f);
        }
        else
        {
            minPeakThreshold   = 0.050f + params.steps * 0.085f;
            requiredCrestRatio = 1.65f + params.steps * 1.35f;
        }

        float targetDb = 0.0f;
        if (envFast[ch] > minPeakThreshold)
        {
            const float crestRatio = envFast[ch] / (envSlow[ch] + 1.0e-5f);
            if (crestRatio > requiredCrestRatio)
            {
                const float excessNorm = juce::jlimit(0.0f, 1.0f, (crestRatio - requiredCrestRatio) / 2.2f);
                const float levelNorm  = juce::jlimit(0.0f, 1.0f, (envFast[ch] - minPeakThreshold) / 0.30f);
                const float transientSeverity = juce::jmax(excessNorm, levelNorm);

                const float maxDb = (externalMaxDb < 0.0f) ? externalMaxDb : maxAttenuationDb;
                targetDb = maxDb * transientSeverity;
            }
        }

        const float gCoeff = (targetDb < smoothedGainDb[ch]) ? gainAtkCoeff : gainRelCoeff;
        smoothedGainDb[ch] = gCoeff * smoothedGainDb[ch] + (1.0f - gCoeff) * targetDb;

        return smoothedGainDb[ch];
    }
};

TransientProcessor::TransientProcessor()
    : impl_(std::make_unique<Impl>())
{
}

TransientProcessor::~TransientProcessor() = default;
TransientProcessor::TransientProcessor(TransientProcessor&&) noexcept = default;
TransientProcessor& TransientProcessor::operator=(TransientProcessor&&) noexcept = default;

void TransientProcessor::prepare(double sampleRate, int maximumBlockSize, int /*numChannels*/)
{
    const double safeSr = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    const int safeBlock = std::max(1, maximumBlockSize);

    impl_->sampleRate = safeSr;

    juce::dsp::ProcessSpec monoSpec {};
    monoSpec.sampleRate       = safeSr;
    monoSpec.maximumBlockSize = static_cast<juce::uint32>(safeBlock);
    monoSpec.numChannels      = 1;

    for (size_t ch = 0; ch < static_cast<size_t>(kMaxChannels); ++ch)
    {
        impl_->crossovers[ch].prepare(monoSpec);
        impl_->crossovers[ch].setType(juce::dsp::LinkwitzRileyFilterType::allpass);
        impl_->crossovers[ch].setCutoffFrequency(impl_->activeSplitHz.load(std::memory_order_relaxed));
    }

    impl_->recalculateParameters();
    reset();
}

void TransientProcessor::setCenterChannelIndex(int centerChannelIdx) noexcept
{
    impl_->centerChannelIdx = std::clamp(centerChannelIdx, 0, kMaxChannels - 1);
}

void TransientProcessor::reset() noexcept
{
    for (size_t ch = 0; ch < static_cast<size_t>(kMaxChannels); ++ch)
    {
        impl_->crossovers[ch].reset();
        impl_->envFast[ch]        = 0.0f;
        impl_->envSlow[ch]        = 0.0f;
        impl_->smoothedGainDb[ch] = 0.0f;
    }
    impl_->lastReductionDb.store(0.0f, std::memory_order_relaxed);
}

void TransientProcessor::updateParameters(const TransientParameters& params) noexcept
{
    impl_->params = params;
    impl_->recalculateParameters();
}

const TransientParameters& TransientProcessor::getParameters() const noexcept
{
    return impl_->params;
}

void TransientProcessor::process(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    float* channels[1] = { buffer };
    process(channels, 1, numSamples);
}

void TransientProcessor::process(std::vector<float>& buffer) noexcept
{
    if (buffer.empty())
        return;

    process(buffer.data(), static_cast<int>(buffer.size()));
}

void TransientProcessor::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    if (impl_->params.bypass)
    {
        impl_->lastReductionDb.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, kMaxChannels));
    float maxBlockReductionDb = 0.0f;

    const bool centerOrMid = impl_->params.useAdvancedMode ?
        (impl_->params.channelMode == TransientChannelMode::CenterOrMidOnly) :
        impl_->params.front;

    // Caso A: Estéreo (2.0) con aislamiento de Mid (Centro acústico)
    if (activeCh == 2 && centerOrMid && channelPointers[0] != nullptr && channelPointers[1] != nullptr)
    {
        float* chL = channelPointers[0];
        float* chR = channelPointers[1];

        for (int i = 0; i < numSamples; ++i)
        {
            const float mid  = 0.5f * (chL[i] + chR[i]);
            const float side = 0.5f * (chL[i] - chR[i]);

            float body = 0.0f;
            float crack = 0.0f;
            impl_->crossovers[0].processSample(0, mid, body, crack);

            const float reductionDb = impl_->evaluateTransientSample(0, crack);
            if (reductionDb < maxBlockReductionDb)
                maxBlockReductionDb = reductionDb;

            const float restoreBoost = impl_->params.restore * 2.0f;
            const float crackGainLin = juce::Decibels::decibelsToGain(reductionDb + restoreBoost);
            const float processedMid = body + (crack * crackGainLin);

            chL[i] = processedMid + side;
            chR[i] = processedMid - side;
        }
    }
    // Caso B: Multicanal Surround / Inmersivo (hasta 16 canales)
    else
    {
        const bool centerOnly = centerOrMid && (activeCh >= 3);
        const int targetCh = impl_->centerChannelIdx;

        for (int i = 0; i < numSamples; ++i)
        {
            float linkedGainDb = 0.0f;
            float body0 = 0.0f, crack0 = 0.0f;
            float body1 = 0.0f, crack1 = 0.0f;
            bool ch0Filtered = false;
            bool ch1Filtered = false;

            if (impl_->params.stereoLinking && activeCh >= 2 && !centerOnly)
            {
                if (channelPointers[0] != nullptr)
                {
                    impl_->crossovers[0].processSample(0, channelPointers[0][i], body0, crack0);
                    ch0Filtered = true;
                }
                if (channelPointers[1] != nullptr)
                {
                    impl_->crossovers[1].processSample(0, channelPointers[1][i], body1, crack1);
                    ch1Filtered = true;
                }

                const float red0 = ch0Filtered ? impl_->evaluateTransientSample(0, crack0) : 0.0f;
                const float red1 = ch1Filtered ? impl_->evaluateTransientSample(1, crack1) : 0.0f;
                linkedGainDb = std::min(red0, red1);
            }

            for (size_t ch = 0; ch < activeCh; ++ch)
            {
                if (channelPointers[ch] == nullptr)
                    continue;

                if (centerOnly && static_cast<int>(ch) != targetCh)
                    continue;

                float body = 0.0f;
                float crack = 0.0f;
                float redDb = 0.0f;

                if (impl_->params.stereoLinking && !centerOnly)
                {
                    if (ch == 0 && ch0Filtered)
                    {
                        body  = body0;
                        crack = crack0;
                        redDb = linkedGainDb;
                    }
                    else if (ch == 1 && ch1Filtered)
                    {
                        body  = body1;
                        crack = crack1;
                        redDb = linkedGainDb;
                    }
                    else
                    {
                        impl_->crossovers[ch].processSample(0, channelPointers[ch][i], body, crack);
                        redDb = impl_->evaluateTransientSample(ch, crack);
                    }
                }
                else
                {
                    impl_->crossovers[ch].processSample(0, channelPointers[ch][i], body, crack);
                    redDb = impl_->evaluateTransientSample(ch, crack);
                }

                if (redDb < maxBlockReductionDb)
                    maxBlockReductionDb = redDb;

                const float restoreBoost = impl_->params.restore * 2.0f;
                const float crackGainLin = juce::Decibels::decibelsToGain(redDb + restoreBoost);
                channelPointers[ch][i]   = body + (crack * crackGainLin);
            }
        }
    }

    impl_->lastReductionDb.store(maxBlockReductionDb, std::memory_order_relaxed);
}

float TransientProcessor::getLastReductionDb() const noexcept
{
    return impl_->lastReductionDb.load(std::memory_order_relaxed);
}

float TransientProcessor::getCurrentSplitHz() const noexcept
{
    return impl_->activeSplitHz.load(std::memory_order_relaxed);
}

} // namespace dash::core
