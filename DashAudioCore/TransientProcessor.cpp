#include "TransientProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace dash::core
{

struct TransientProcessor::Impl
{
    double sampleRate       = 48000.0;
    int    centerChannelIdx = 2;
    TransientParameters params {};

    juce::dsp::LinkwitzRileyFilter<float> bodyCrackCrossover;

    std::array<float, static_cast<size_t>(kMaxChannels)> envFast {};
    std::array<float, static_cast<size_t>(kMaxChannels)> envSlow {};
    std::array<float, static_cast<size_t>(kMaxChannels)> smoothedGainDb {};

    float fastAtkCoeff = 0.9f;
    float fastRelCoeff = 0.999f;
    float slowAtkCoeff = 0.99f;
    float slowRelCoeff = 0.9995f;
    float gainAtkCoeff = 0.9f;
    float gainRelCoeff = 0.999f;

    float maxAttenuationDb = -6.93575f; // Gun = 0.45 -> -6.94 dB (~ -7 dB)
    float lastReductionDb  = 0.0f;

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.00005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    void recalculateParameters() noexcept
    {
        fastAtkCoeff = calcOnePoleCoeff(0.00015f);

        const float relSec = 0.020f + juce::jlimit(0.0f, 1.0f, params.release) * 0.160f;
        fastRelCoeff = calcOnePoleCoeff(relSec);

        slowAtkCoeff = calcOnePoleCoeff(0.018f);
        slowRelCoeff = calcOnePoleCoeff(0.220f);

        gainAtkCoeff = calcOnePoleCoeff(0.00020f);
        gainRelCoeff = calcOnePoleCoeff(relSec);

        // Calibración de atenuación táctica de nuestro DSP: Gun = 0.45 -> 20*log10(0.45) = -6.9357 dB
        const float clampedGun = juce::jlimit(0.10f, 1.0f, params.gun);
        maxAttenuationDb = juce::Decibels::gainToDecibels(clampedGun, -24.0f);
    }

    float processChannelSample(int ch, float& sample) noexcept
    {
        float gunBody  = 0.0f;
        float gunCrack = 0.0f;

        bodyCrackCrossover.processSample(ch, sample, gunBody, gunCrack);

        const float absCrack = std::abs(gunCrack);
        const size_t idx = static_cast<size_t>(ch);

        const float fCoeff = (absCrack > envFast[idx]) ? fastAtkCoeff : fastRelCoeff;
        envFast[idx] = fCoeff * envFast[idx] + (1.0f - fCoeff) * absCrack;

        const float sCoeff = (absCrack > envSlow[idx]) ? slowAtkCoeff : slowRelCoeff;
        envSlow[idx] = sCoeff * envSlow[idx] + (1.0f - sCoeff) * absCrack;

        const float minPeakThreshold   = 0.050f + params.steps * 0.085f;
        const float requiredCrestRatio = 1.65f + params.steps * 1.35f;

        float targetDb = 0.0f;
        if (envFast[idx] > minPeakThreshold)
        {
            const float crestRatio = envFast[idx] / (envSlow[idx] + 1.0e-5f);
            if (crestRatio > requiredCrestRatio)
            {
                const float excessNorm = juce::jlimit(0.0f, 1.0f, (crestRatio - requiredCrestRatio) / 2.5f);
                const float levelNorm  = juce::jlimit(0.0f, 1.0f, (envFast[idx] - minPeakThreshold) / 0.25f);
                const float transientSeverity = juce::jmax(excessNorm, levelNorm);
                targetDb = maxAttenuationDb * transientSeverity;
            }
        }

        const float gCoeff = (targetDb < smoothedGainDb[idx]) ? gainAtkCoeff : gainRelCoeff;
        smoothedGainDb[idx] = gCoeff * smoothedGainDb[idx] + (1.0f - gCoeff) * targetDb;

        const float restoreBoostDb =
            params.restore * 2.5f * (1.0f - juce::jlimit(0.0f, 1.0f, -smoothedGainDb[idx] / 6.0f));
        const float crackGainLin = juce::Decibels::decibelsToGain(smoothedGainDb[idx] + restoreBoostDb);

        sample = gunBody + (gunCrack * crackGainLin);
        return smoothedGainDb[idx];
    }
};

TransientProcessor::TransientProcessor()
    : impl_(std::make_unique<Impl>())
{
}

TransientProcessor::~TransientProcessor() = default;
TransientProcessor::TransientProcessor(TransientProcessor&&) noexcept = default;
TransientProcessor& TransientProcessor::operator=(TransientProcessor&&) noexcept = default;

void TransientProcessor::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    (void)numChannels;
    impl_->sampleRate = (sampleRate > 8000.0) ? sampleRate : 48000.0;

    juce::dsp::ProcessSpec filterSpec {};
    filterSpec.sampleRate       = impl_->sampleRate;
    filterSpec.maximumBlockSize = static_cast<juce::uint32>(std::max(1, maximumBlockSize));
    filterSpec.numChannels      = static_cast<juce::uint32>(kMaxChannels);

    impl_->bodyCrackCrossover.prepare(filterSpec);
    impl_->bodyCrackCrossover.setCutoffFrequency(kBodyCrackCrossoverHz);

    impl_->recalculateParameters();
    reset();
}

void TransientProcessor::setCenterChannelIndex(int centerChannelIdx) noexcept
{
    impl_->centerChannelIdx = (centerChannelIdx >= 0 && centerChannelIdx < kMaxChannels) ? centerChannelIdx : 2;
}

void TransientProcessor::reset() noexcept
{
    impl_->bodyCrackCrossover.reset();
    impl_->envFast.fill(0.0f);
    impl_->envSlow.fill(0.0f);
    impl_->smoothedGainDb.fill(0.0f);
    impl_->lastReductionDb = 0.0f;
}

void TransientProcessor::updateParameters(const TransientParameters& params) noexcept
{
    impl_->params = params;
    impl_->recalculateParameters();
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

    if (impl_->params.bypass)
    {
        impl_->lastReductionDb = 0.0f;
        return;
    }

    const int activeCh = std::min(numChannels, kMaxChannels);
    float peakReductionDb = 0.0f;

    // Caso A: Multicanal 5.1 / 7.1 -> Aislar Canal Central (o trío frontal si Front == false)
    if (activeCh >= 6)
    {
        const int cIdx = (impl_->centerChannelIdx >= 0 && impl_->centerChannelIdx < activeCh)
            ? impl_->centerChannelIdx
            : 2;

        if (impl_->params.front)
        {
            float* centerData = channelPointers[cIdx];
            if (centerData != nullptr)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    const float grDb = impl_->processChannelSample(cIdx, centerData[i]);
                    if (grDb < peakReductionDb)
                        peakReductionDb = grDb;
                }
            }
        }
        else
        {
            const std::array<int, 3> targetChannels { 0, 1, cIdx };
            for (int ch : targetChannels)
            {
                if (ch >= 0 && ch < activeCh && channelPointers[ch] != nullptr)
                {
                    float* chData = channelPointers[ch];
                    for (int i = 0; i < numSamples; ++i)
                    {
                        const float grDb = impl_->processChannelSample(ch, chData[i]);
                        if (grDb < peakReductionDb)
                            peakReductionDb = grDb;
                    }
                }
            }
        }
    }
    // Caso B: Estéreo (2.0) -> Si Front == 1, aísla el Centro Fantasma Mid = (L+R)*0.5 dejando Side intacto
    else if (activeCh == 2 && impl_->params.front && channelPointers[0] != nullptr && channelPointers[1] != nullptr)
    {
        float* leftData  = channelPointers[0];
        float* rightData = channelPointers[1];

        for (int i = 0; i < numSamples; ++i)
        {
            float mid        = 0.5f * (leftData[i] + rightData[i]);
            const float side = 0.5f * (leftData[i] - rightData[i]);

            const float grDb = impl_->processChannelSample(0, mid);
            if (grDb < peakReductionDb)
                peakReductionDb = grDb;

            leftData[i]  = mid + side;
            rightData[i] = mid - side;
        }
    }
    // Caso C: Mono o Estéreo completo (Front == 0)
    else
    {
        for (int ch = 0; ch < activeCh; ++ch)
        {
            float* chData = channelPointers[ch];
            if (chData != nullptr)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    const float grDb = impl_->processChannelSample(ch, chData[i]);
                    if (grDb < peakReductionDb)
                        peakReductionDb = grDb;
                }
            }
        }
    }

    impl_->bodyCrackCrossover.snapToZero();
    impl_->lastReductionDb = peakReductionDb;
}

float TransientProcessor::getLastReductionDb() const noexcept
{
    return impl_->lastReductionDb;
}

} // namespace dash::core
