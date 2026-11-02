#include "ParametricEQProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <numbers>

namespace dash::core
{

namespace
{
    constexpr float kPi = std::numbers::pi_v<float>;

    struct BiquadCoeffs
    {
        float b0 = 1.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
    };

    BiquadCoeffs calculateBiquadCoeffs(
        ParametricFilterType type,
        float freqHz,
        float gainDb,
        float q,
        double sampleRate) noexcept
    {
        BiquadCoeffs c {};
        const float nyquist = static_cast<float>(sampleRate * 0.49);
        const float f0 = std::clamp(freqHz, 20.0f, nyquist);
        const float safeQ = std::max(0.1f, q);

        const float w0 = 2.0f * kPi * f0 / static_cast<float>(sampleRate);
        const float cosW = std::cos(w0);
        const float sinW = std::sin(w0);
        const float alpha = sinW / (2.0f * safeQ);
        const float A = std::pow(10.0f, gainDb / 40.0f); // sqrt(gain lineal)

        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

        switch (type)
        {
            case ParametricFilterType::Peaking:
            {
                b0 = 1.0f + alpha * A;
                b1 = -2.0f * cosW;
                b2 = 1.0f - alpha * A;
                a0 = 1.0f + alpha / A;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha / A;
                break;
            }
            case ParametricFilterType::LowShelf:
            {
                const float sqrtA = std::sqrt(A);
                b0 = A * ((A + 1.0f) - (A - 1.0f) * cosW + 2.0f * sqrtA * alpha);
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosW - 2.0f * sqrtA * alpha);
                a0 = (A + 1.0f) + (A - 1.0f) * cosW + 2.0f * sqrtA * alpha;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosW);
                a2 = (A + 1.0f) + (A - 1.0f) * cosW - 2.0f * sqrtA * alpha;
                break;
            }
            case ParametricFilterType::HighShelf:
            {
                const float sqrtA = std::sqrt(A);
                b0 = A * ((A + 1.0f) + (A - 1.0f) * cosW + 2.0f * sqrtA * alpha);
                b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosW - 2.0f * sqrtA * alpha);
                a0 = (A + 1.0f) - (A - 1.0f) * cosW + 2.0f * sqrtA * alpha;
                a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosW);
                a2 = (A + 1.0f) - (A - 1.0f) * cosW - 2.0f * sqrtA * alpha;
                break;
            }
            case ParametricFilterType::HighPass:
            {
                b0 = (1.0f + cosW) * 0.5f;
                b1 = -(1.0f + cosW);
                b2 = (1.0f + cosW) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha;
                break;
            }
            case ParametricFilterType::LowPass:
            {
                b0 = (1.0f - cosW) * 0.5f;
                b1 = 1.0f - cosW;
                b2 = (1.0f - cosW) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha;
                break;
            }
            case ParametricFilterType::Notch:
            {
                b0 = 1.0f;
                b1 = -2.0f * cosW;
                b2 = 1.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosW;
                a2 = 1.0f - alpha;
                break;
            }
        }

        const float invA0 = (std::abs(a0) > 1.0e-8f) ? (1.0f / a0) : 1.0f;
        c.b0 = b0 * invA0;
        c.b1 = b1 * invA0;
        c.b2 = b2 * invA0;
        c.a1 = a1 * invA0;
        c.a2 = a2 * invA0;

        return c;
    }
} // namespace

struct ParametricEQProcessor::Impl
{
    double sampleRate = 48000.0;
    std::atomic<bool> bypass { false };
    std::atomic<bool> hasProcessedAudio { false };
    std::atomic<bool> pendingUpdate { false };

    std::array<ParametricBandParameters, kMaxBands> bandParams {};
    std::array<ParametricBandParameters, kMaxBands> stagingParams {};
    std::array<BiquadCoeffs, kMaxBands> stagingCoeffs {};
    std::array<BiquadCoeffs, kMaxBands> targetCoeffs {};
    std::array<BiquadCoeffs, kMaxBands> currentCoeffs {};

    // Estados DF2T: s1 y s2 por cada banda y por cada canal
    struct DF2TState
    {
        float s1 = 0.0f;
        float s2 = 0.0f;
    };
    std::array<std::array<DF2TState, kMaxBands>, kMaxChannels> states {};

    // Ducking Inverso Anti-Disparos: Realce dinámico temporal en la banda de pasos (3.2 kHz a 4.5 kHz)
    std::atomic<int> dynamicBoostSamplesRemaining { 0 };
    std::atomic<float> dynamicBoostTargetDb { 0.0f };
    float currentDynamicBoostDb = 0.0f;
    BiquadCoeffs dynamicFootstepCoeffs { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<DF2TState, kMaxChannels> dynamicFootstepStates {};

    void recalculateBand(size_t bandIdx) noexcept
    {
        if (bandIdx >= kMaxBands)
            return;

        const auto& p = bandParams[bandIdx];
        if (!p.enabled)
        {
            targetCoeffs[bandIdx] = BiquadCoeffs { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        }
        else
        {
            targetCoeffs[bandIdx] = calculateBiquadCoeffs(
                p.type, p.frequencyHz, p.gainDb, p.q, sampleRate);
        }
        stagingCoeffs[bandIdx] = targetCoeffs[bandIdx];
        stagingParams[bandIdx] = bandParams[bandIdx];
    }
};

ParametricEQProcessor::ParametricEQProcessor()
    : impl_(std::make_unique<Impl>())
{
    for (size_t i = 0; i < kMaxBands; ++i)
    {
        impl_->currentCoeffs[i] = BiquadCoeffs { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        impl_->targetCoeffs[i]  = BiquadCoeffs { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    }
}

ParametricEQProcessor::~ParametricEQProcessor() = default;
ParametricEQProcessor::ParametricEQProcessor(ParametricEQProcessor&&) noexcept = default;
ParametricEQProcessor& ParametricEQProcessor::operator=(ParametricEQProcessor&&) noexcept = default;

void ParametricEQProcessor::prepare(double sampleRate, int /*maximumBlockSize*/, int /*numChannels*/)
{
    impl_->sampleRate = (sampleRate > 8000.0) ? sampleRate : 48000.0;

    for (size_t i = 0; i < kMaxBands; ++i)
    {
        impl_->recalculateBand(i);
        impl_->currentCoeffs[i] = impl_->targetCoeffs[i];
    }

    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        impl_->dynamicFootstepStates[ch].s1 = 0.0f;
        impl_->dynamicFootstepStates[ch].s2 = 0.0f;
    }
    impl_->dynamicBoostSamplesRemaining.store(0, std::memory_order_relaxed);
    impl_->currentDynamicBoostDb = 0.0f;
    impl_->dynamicFootstepCoeffs = calculateBiquadCoeffs(
        ParametricFilterType::Peaking, 3800.0f, 0.0f, 2.8f, impl_->sampleRate);

    reset();
}

void ParametricEQProcessor::reset() noexcept
{
    impl_->hasProcessedAudio.store(false, std::memory_order_relaxed);
    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        for (size_t b = 0; b < kMaxBands; ++b)
        {
            impl_->states[ch][b].s1 = 0.0f;
            impl_->states[ch][b].s2 = 0.0f;
        }
        impl_->dynamicFootstepStates[ch].s1 = 0.0f;
        impl_->dynamicFootstepStates[ch].s2 = 0.0f;
    }
    impl_->dynamicBoostSamplesRemaining.store(0, std::memory_order_relaxed);
    impl_->currentDynamicBoostDb = 0.0f;
}

void ParametricEQProcessor::setBand(size_t bandIndex, const ParametricBandParameters& params) noexcept
{
    if (bandIndex >= kMaxBands)
        return;

    impl_->stagingParams[bandIndex] = params;
    if (!params.enabled)
    {
        impl_->stagingCoeffs[bandIndex] = BiquadCoeffs { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    }
    else
    {
        impl_->stagingCoeffs[bandIndex] = calculateBiquadCoeffs(
            params.type, params.frequencyHz, params.gainDb, params.q, impl_->sampleRate);
    }

    if (!impl_->hasProcessedAudio.load(std::memory_order_relaxed))
    {
        impl_->bandParams[bandIndex] = impl_->stagingParams[bandIndex];
        impl_->targetCoeffs[bandIndex] = impl_->stagingCoeffs[bandIndex];
        impl_->currentCoeffs[bandIndex] = impl_->stagingCoeffs[bandIndex];
    }
    else
    {
        impl_->pendingUpdate.store(true, std::memory_order_release);
    }
}

ParametricBandParameters ParametricEQProcessor::getBand(size_t bandIndex) const noexcept
{
    if (bandIndex >= kMaxBands)
        return {};

    return impl_->bandParams[bandIndex];
}

void ParametricEQProcessor::setBypass(bool bypass) noexcept
{
    impl_->bypass.store(bypass, std::memory_order_relaxed);
}

bool ParametricEQProcessor::isBypassed() const noexcept
{
    return impl_->bypass.load(std::memory_order_relaxed);
}

void ParametricEQProcessor::triggerGunshotFootstepBoost(float boostDb, float durationMs) noexcept
{
    const int durationSamples = static_cast<int>(std::round((durationMs * 0.001) * impl_->sampleRate));
    impl_->dynamicBoostTargetDb.store(std::clamp(boostDb, 0.0f, 12.0f), std::memory_order_relaxed);
    const int currentRemaining = impl_->dynamicBoostSamplesRemaining.load(std::memory_order_relaxed);
    if (durationSamples > currentRemaining)
        impl_->dynamicBoostSamplesRemaining.store(durationSamples, std::memory_order_release);
}

float ParametricEQProcessor::getDynamicFootstepBoostDb() const noexcept
{
    return impl_->currentDynamicBoostDb;
}

void ParametricEQProcessor::process(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    float* channels[1] = { buffer };
    process(channels, 1, numSamples);
}

void ParametricEQProcessor::process(std::vector<float>& buffer) noexcept
{
    if (buffer.empty())
        return;

    process(buffer.data(), static_cast<int>(buffer.size()));
}

void ParametricEQProcessor::process(float* const* channelPointers, int numChannels, int numSamples) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    // 1. Manejo de Ducking Inverso de Pasos (3.8 kHz Peaking Boost)
    int remaining = impl_->dynamicBoostSamplesRemaining.load(std::memory_order_acquire);
    float targetGain = 0.0f;
    if (remaining > 0)
    {
        targetGain = impl_->dynamicBoostTargetDb.load(std::memory_order_relaxed);
        remaining = std::max(0, remaining - numSamples);
        impl_->dynamicBoostSamplesRemaining.store(remaining, std::memory_order_release);
    }

    // Suavizado dinámico de ganancia del filtro (ataque rápido 0.35, relajación suave 0.03)
    const float smoothCoeff = (targetGain > impl_->currentDynamicBoostDb) ? 0.35f : 0.03f;
    impl_->currentDynamicBoostDb += smoothCoeff * (targetGain - impl_->currentDynamicBoostDb);

    const bool hasDynamicBoost = (impl_->currentDynamicBoostDb > 0.05f);
    if (hasDynamicBoost)
    {
        impl_->dynamicFootstepCoeffs = calculateBiquadCoeffs(
            ParametricFilterType::Peaking, 3800.0f, impl_->currentDynamicBoostDb, 2.8f, impl_->sampleRate);
    }

    const bool staticBypassed = impl_->bypass.load(std::memory_order_relaxed);
    if (staticBypassed && !hasDynamicBoost)
        return;

    // Sincronización lock-free atómica exclusivamente al inicio del bloque de audio
    if (!staticBypassed && impl_->pendingUpdate.load(std::memory_order_acquire))
    {
        for (size_t b = 0; b < kMaxBands; ++b)
        {
            impl_->bandParams[b]   = impl_->stagingParams[b];
            impl_->targetCoeffs[b] = impl_->stagingCoeffs[b];
        }
        impl_->pendingUpdate.store(false, std::memory_order_release);
    }

    const size_t activeCh = static_cast<size_t>(std::min(numChannels, static_cast<int>(kMaxChannels)));

    // Si es el primer bloque tras reset/prepare, sincronizar inmediatamente
    if (!staticBypassed)
    {
        if (!impl_->hasProcessedAudio.load(std::memory_order_relaxed))
        {
            for (size_t b = 0; b < kMaxBands; ++b)
                impl_->currentCoeffs[b] = impl_->targetCoeffs[b];
            impl_->hasProcessedAudio.store(true, std::memory_order_relaxed);
        }
        else
        {
            // Suavizado dinámico suave ante cambios de parámetros en caliente
            constexpr float kSmooth = 0.25f;
            for (size_t b = 0; b < kMaxBands; ++b)
            {
                auto& cur = impl_->currentCoeffs[b];
                const auto& tgt = impl_->targetCoeffs[b];

                cur.b0 += kSmooth * (tgt.b0 - cur.b0);
                cur.b1 += kSmooth * (tgt.b1 - cur.b1);
                cur.b2 += kSmooth * (tgt.b2 - cur.b2);
                cur.a1 += kSmooth * (tgt.a1 - cur.a1);
                cur.a2 += kSmooth * (tgt.a2 - cur.a2);
            }
        }
    }

    // 2. Procesamiento de canales de audio
    for (size_t ch = 0; ch < activeCh; ++ch)
    {
        float* chData = channelPointers[ch];
        if (chData == nullptr)
            continue;

        // Etapa A: Bandas paramétricas estáticas (si no están en bypass)
        if (!staticBypassed)
        {
            for (size_t b = 0; b < kMaxBands; ++b)
            {
                if (!impl_->bandParams[b].enabled)
                    continue;

                const auto& c = impl_->currentCoeffs[b];
                auto& st = impl_->states[ch][b];

                for (int i = 0; i < numSamples; ++i)
                {
                    const float x = chData[i];
                    // Direct Form II Transposed (DF2T)
                    const float y = c.b0 * x + st.s1;
                    st.s1 = c.b1 * x - c.a1 * y + st.s2;
                    st.s2 = c.b2 * x - c.a2 * y;
                    chData[i] = y;
                }
            }
        }

        // Etapa B: Ducking Inverso Anti-Disparos (Detección de disparos y realce dinámico de pasos)
        if (hasDynamicBoost)
        {
            const auto& dynC = impl_->dynamicFootstepCoeffs;
            auto& dynSt = impl_->dynamicFootstepStates[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                const float x = chData[i];
                const float y = dynC.b0 * x + dynSt.s1;
                dynSt.s1 = dynC.b1 * x - dynC.a1 * y + dynSt.s2;
                dynSt.s2 = dynC.b2 * x - dynC.a2 * y;
                chData[i] = y;
            }
        }
    }
}

} // namespace dash::core
