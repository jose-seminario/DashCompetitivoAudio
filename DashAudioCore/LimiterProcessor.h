#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

struct LimiterParameters
{
    float ceilingDb   = -0.5f;     // Techo de seguridad en dBFS (-0.5 dBFS => ~0.944061 lineal)
    float releaseNorm = 0.612903f; // [0.0 .. 1.0] Relajación del limitador (15 ms .. 250 ms)
    bool  bypass      = false;
};

/**
 * Limitador de techo de muestra exacta en tiempo real (LimiterProcessor).
 *
 * API pública en C++20 estándar libre de dependencias de JUCE o VST.
 * Cero asignaciones dinámicas dentro de process() / processSampleGroup().
 */
class LimiterProcessor
{
public:
    static constexpr size_t kMaxChannels = 8;

    LimiterProcessor();
    ~LimiterProcessor();

    LimiterProcessor(LimiterProcessor&&) noexcept;
    LimiterProcessor& operator=(LimiterProcessor&&) noexcept;

    LimiterProcessor(const LimiterProcessor&) = delete;
    LimiterProcessor& operator=(const LimiterProcessor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;
    void updateParameters(const LimiterParameters& params) noexcept;

    /** Procesa un bloque mono en sitio (API estándar C++). */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga estándar C++ con std::vector<float> (sin reasignaciones). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1.0, 2.0, 5.1, 7.1) con enlace de pico entre canales. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    /**
     * Evalúa el pico instantáneo multicanal y actualiza la ganancia lineal del limitador
     * para una muestra individual (usado en bucle por DynamicsProcessor).
     */
    [[nodiscard]] float computeSampleGainLinear(float peakAmplitude) noexcept;

    [[nodiscard]] float getCurrentGainLinear() const noexcept;
    [[nodiscard]] float getCurrentReductionDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
