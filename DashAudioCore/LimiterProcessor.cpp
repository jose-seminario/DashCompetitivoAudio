#include "LimiterProcessor.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <atomic>
#include <cmath>

namespace dash::core
{

struct LimiterProcessor::Impl
{
    double sampleRate     = 48000.0;
    int    maxBlockSize   = 512;
    size_t activeChannels = 2;
    LimiterParameters params {};

    float ceilingLin   = 0.94406088f; // -0.5 dBFS
    float releaseCoeff = 0.999f;
    float gainLin      = 1.0f;

    std::atomic<float> telemetryGainLin { 1.0f };

    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate)));
    }

    void recalculateCoefficients() noexcept
    {
        ceilingLin = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 0.0f, params.ceilingDb));
        const float relSec = juce::jlimit(0.005f, 0.500f, params.releaseMs * 0.001f);
        releaseCoeff = calcOnePoleCoeff(relSec);
    }
};

LimiterProcessor::LimiterProcessor()
    : impl_(std::make_unique<Impl>())
{
    impl_->recalculateCoefficients();
}

LimiterProcessor::~LimiterProcessor() = default;
LimiterProcessor::LimiterProcessor(LimiterProcessor&&) noexcept = default;
LimiterProcessor& LimiterProcessor::operator=(LimiterProcessor&&) noexcept = default;

void LimiterProcessor::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    impl_->sampleRate     = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    impl_->maxBlockSize   = std::max(1, maximumBlockSize);
    impl_->activeChannels = static_cast<size_t>(std::clamp(numChannels, 1, static_cast<int>(kMaxChannels)));
    impl_->recalculateCoefficients();
    reset();
}

void LimiterProcessor::reset() noexcept
{
    impl_->gainLin = 1.0f;
    impl_->telemetryGainLin.store(1.0f, std::memory_order_relaxed);
}

void LimiterProcessor::updateParameters(const LimiterParameters& params) noexcept
{
    impl_->params = params;
    impl_->recalculateCoefficients();
}

const LimiterParameters& LimiterProcessor::getParameters() const noexcept
{
    return impl_->params;
}

float LimiterProcessor::computeSampleGainLinear(float peakAmplitude) noexcept
{
    if (impl_->params.bypass)
    {
        impl_->gainLin = 1.0f;
        return 1.0f;
    }

    float desiredGain = 1.0f;
    if (peakAmplitude > impl_->ceilingLin && peakAmplitude > 1.0e-12f)
        desiredGain = impl_->ceilingLin / peakAmplitude;

    if (desiredGain <= impl_->gainLin)
        impl_->gainLin = desiredGain;
    else
        impl_->gainLin = impl_->releaseCoeff * impl_->gainLin + (1.0f - impl_->releaseCoeff) * 1.0f;

    return impl_->gainLin;
}

void LimiterProcessor::process(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    float* channels[1] = { buffer };
    process(channels, 1, numSamples);
}

void LimiterProcessor::process(std::vector<float>& buffer) noexcept
{
    if (buffer.empty())
        return;

    process(buffer.data(), static_cast<int>(buffer.size()));
}

void LimiterProcessor::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    if (impl_->params.bypass)
    {
        impl_->gainLin = 1.0f;
        impl_->telemetryGainLin.store(1.0f, std::memory_order_relaxed);
        return;
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));
    const float ceiling   = impl_->ceilingLin;

    for (int i = 0; i < numSamples; ++i)
    {
        float mainPeak = 0.0f;
        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] != nullptr)
            {
                const float absSample = std::abs(channelPointers[ch][i]);
                if (absSample > mainPeak)
                    mainPeak = absSample;
            }
        }

        const float gMain = computeSampleGainLinear(mainPeak);

        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (channelPointers[ch] != nullptr)
            {
                const float limitedSample = channelPointers[ch][i] * gMain;
                channelPointers[ch][i] = std::clamp(limitedSample, -ceiling, ceiling);
            }
        }
    }

    impl_->telemetryGainLin.store(impl_->gainLin, std::memory_order_relaxed);
}

float LimiterProcessor::getCurrentGainLinear() const noexcept
{
    return impl_->telemetryGainLin.load(std::memory_order_relaxed);
}

float LimiterProcessor::getCurrentReductionDb() const noexcept
{
    return juce::Decibels::gainToDecibels(impl_->telemetryGainLin.load(std::memory_order_relaxed), -120.0f);
}

} // namespace dash::core
