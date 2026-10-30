#include "ConvolutionEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>

namespace dash::spatial
{

struct ConvolutionEngine::Impl
{
    double sampleRate     = 48000.0;
    int    maxBlockSize   = 512;
    std::atomic<size_t> filterLength { 256 };

    // Doble buffer para conmutación lock-free de filtros
    struct FilterBank
    {
        std::array<std::vector<float>, kMaxChannels> leftFilters;
        std::array<std::vector<float>, kMaxChannels> rightFilters;
        std::array<bool, kMaxChannels> channelHasFilter {};
    };

    std::array<FilterBank, 2> banks;
    std::atomic<int> activeBank { 0 };

    // Búferes de historial para convolución continua de muestra exacta
    std::array<std::vector<float>, kMaxChannels> history;
    size_t historyPos = 0;
};

ConvolutionEngine::ConvolutionEngine()
    : impl_(std::make_unique<Impl>())
{
}

ConvolutionEngine::~ConvolutionEngine() = default;
ConvolutionEngine::ConvolutionEngine(ConvolutionEngine&&) noexcept = default;
ConvolutionEngine& ConvolutionEngine::operator=(ConvolutionEngine&&) noexcept = default;

void ConvolutionEngine::prepare(double sampleRate, int maximumBlockSize, size_t maxFilterLength)
{
    impl_->sampleRate     = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    impl_->maxBlockSize   = std::max(1, maximumBlockSize);
    impl_->filterLength.store(std::clamp(maxFilterLength, size_t(32), kMaxFilterLength), std::memory_order_relaxed);

    const size_t historyCapacity = static_cast<size_t>(impl_->maxBlockSize) + kMaxFilterLength + 64;

    for (size_t b = 0; b < 2; ++b)
    {
        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            impl_->banks[b].leftFilters[ch].assign(kMaxFilterLength, 0.0f);
            impl_->banks[b].rightFilters[ch].assign(kMaxFilterLength, 0.0f);
            impl_->banks[b].channelHasFilter[ch] = false;
        }
    }

    for (size_t ch = 0; ch < kMaxChannels; ++ch)
        impl_->history[ch].assign(historyCapacity, 0.0f);

    reset();
}

void ConvolutionEngine::reset() noexcept
{
    for (size_t ch = 0; ch < kMaxChannels; ++ch)
        std::fill(impl_->history[ch].begin(), impl_->history[ch].end(), 0.0f);
    impl_->historyPos = 0;
}

bool ConvolutionEngine::loadFilterPair(
    size_t channelIndex,
    const float* leftEarIR,
    const float* rightEarIR,
    size_t filterLength)
{
    if (channelIndex >= kMaxChannels || leftEarIR == nullptr || rightEarIR == nullptr || filterLength == 0)
        return false;

    // Escribir en el banco inactivo para evitar condiciones de carrera con el audio thread
    const int inactiveIdx = 1 - impl_->activeBank.load(std::memory_order_relaxed);
    auto& bank = impl_->banks[static_cast<size_t>(inactiveIdx)];

    const size_t taps = std::min(filterLength, kMaxFilterLength);
    bank.leftFilters[channelIndex].assign(leftEarIR, leftEarIR + taps);
    bank.rightFilters[channelIndex].assign(rightEarIR, rightEarIR + taps);
    bank.channelHasFilter[channelIndex] = true;

    size_t curLen = impl_->filterLength.load(std::memory_order_relaxed);
    while (taps > curLen && !impl_->filterLength.compare_exchange_weak(curLen, taps, std::memory_order_relaxed))
    {
    }

    return true;
}

void ConvolutionEngine::commitFilters() noexcept
{
    // Conmutación atómica lock-free del banco activo
    const int nextBank = 1 - impl_->activeBank.load(std::memory_order_relaxed);
    impl_->activeBank.store(nextBank, std::memory_order_release);
}

void ConvolutionEngine::convolveAndAccumulate(
    size_t channelIndex,
    const float* input,
    float* outputLeft,
    float* outputRight,
    int numSamples) noexcept
{
    if (channelIndex >= kMaxChannels || input == nullptr || outputLeft == nullptr || outputRight == nullptr || numSamples <= 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    const int bankIdx = impl_->activeBank.load(std::memory_order_acquire);
    const auto& bank = impl_->banks[static_cast<size_t>(bankIdx)];

    if (!bank.channelHasFilter[channelIndex])
        return; // Canal sin filtro asignado

    const auto& hL = bank.leftFilters[channelIndex];
    const auto& hR = bank.rightFilters[channelIndex];
    const size_t numTaps = std::min(hL.size(), hR.size());
    if (numTaps == 0)
        return;

    auto& hist = impl_->history[channelIndex];
    const size_t histTail = numTaps - 1;

    // Procesar en fragmentos acotados a maxBlockSize para evitar desbordamiento de búfer
    int samplesProcessed = 0;
    while (samplesProcessed < numSamples)
    {
        const int chunkSize = std::min(numSamples - samplesProcessed, impl_->maxBlockSize);
        if (histTail + static_cast<size_t>(chunkSize) > hist.size())
            break;

        const float* inChunk = input + samplesProcessed;
        float* outLChunk = outputLeft + samplesProcessed;
        float* outRChunk = outputRight + samplesProcessed;

        for (int i = 0; i < chunkSize; ++i)
            hist[histTail + static_cast<size_t>(i)] = inChunk[i];

        for (int n = 0; n < chunkSize; ++n)
        {
            const size_t offset = histTail + static_cast<size_t>(n);
            float sumL = 0.0f;
            float sumR = 0.0f;

            for (size_t k = 0; k < numTaps; ++k)
            {
                const float x = hist[offset - k];
                sumL += x * hL[k];
                sumR += x * hR[k];
            }

            outLChunk[n] += sumL;
            outRChunk[n] += sumR;
        }

        for (size_t k = 0; k < histTail; ++k)
            hist[k] = hist[static_cast<size_t>(chunkSize) + k];

        samplesProcessed += chunkSize;
    }
}

size_t ConvolutionEngine::getFilterLength() const noexcept
{
    return impl_->filterLength.load(std::memory_order_relaxed);
}

bool ConvolutionEngine::hasFilters(size_t channelIndex) const noexcept
{
    if (channelIndex >= kMaxChannels)
        return false;
    const int bankIdx = impl_->activeBank.load(std::memory_order_relaxed);
    return impl_->banks[static_cast<size_t>(bankIdx)].channelHasFilter[channelIndex];
}

} // namespace dash::spatial
