#pragma once

#include "AudioConstants.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Parámetros del Sample Peak Limiter (-0.5 dBFS).
 * Diseñado para latencia cero estricta (0 muestras / 0.0 ms) en estéreo competitivo.
 */
struct LimiterParameters
{
    float ceilingDb    = kLimiterCeilingDb; // Techo estricto en dBFS (-0.5 dBFS => ~0.944061 lineal)
    float releaseMs    = 50.0f;             // [5.0 .. 500.0 ms] Tiempo de relajación
    bool  bypass       = false;
    bool  linkChannels = true;              // Enlace estéreo L/R para conservar imagen intacta
};

/**
 * Sample Peak Limiter (-0.5 dBFS) en Tiempo Real (LimiterProcessor).
 * Soporta 1 o 2 canales estéreo con latencia cero (0 muestras).
 */
class LimiterProcessor
{
public:
    static constexpr size_t kMaxChannels = kMaxSupportedChannels;

    LimiterProcessor();
    ~LimiterProcessor();

    LimiterProcessor(LimiterProcessor&&) noexcept;
    LimiterProcessor& operator=(LimiterProcessor&&) noexcept;

    LimiterProcessor(const LimiterProcessor&) = delete;
    LimiterProcessor& operator=(const LimiterProcessor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;
    void updateParameters(const LimiterParameters& params) noexcept;
    [[nodiscard]] const LimiterParameters& getParameters() const noexcept;

    void process(float* buffer, int numSamples) noexcept;
    void process(std::vector<float>& buffer) noexcept;
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    [[nodiscard]] float computeSampleGainLinear(float peakAmplitude) noexcept;
    [[nodiscard]] float getCurrentGainLinear() const noexcept;
    [[nodiscard]] float getCurrentReductionDb() const noexcept;

    [[nodiscard]] bool isTruePeak() const noexcept { return false; }
    [[nodiscard]] int  getLatencySamples() const noexcept { return 0; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
