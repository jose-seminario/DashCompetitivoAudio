#pragma once

#include "AudioConstants.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Parámetros del Limitador Brickwall de Muestra Exacta (Sample-Peak Limiter).
 * Diseñado para latencia cero estricta (0.0 ms) en audio competitivo.
 */
struct LimiterParameters
{
    float ceilingDb           = -0.5f;     // Techo estricto en dBFS (-0.5 dBFS => ~0.944061 lineal)
    float releaseNorm         = 0.612903f; // [0.0 .. 1.0] Relajación normalizada (15 ms .. 250 ms)
    bool  bypass              = false;

    // Parámetros acústicos avanzados
    float releaseMs           = 50.0f;     // [5.0 .. 500.0 ms] Tiempo de relajación de ganancia
    bool  linkChannels        = true;      // true = enlace multicanal (evita desplazamientos del panorama)
    bool  includeLfeInPeakLink= false;     // false = LFE se limita individualmente para no atenuar estéreo
    bool  useAdvancedMode     = false;     // true = usa releaseMs; false = usa releaseNorm canónico
};

/**
 * Limitador Brickwall de Muestra Exacta en Tiempo Real (LimiterProcessor).
 * Soporta de 1 a 16 canales simultáneos (kMaxSupportedChannels = 16).
 *
 * NOTA TÉCNICA DE ARQUITECTURA DSP:
 * Este procesador es estrictamente un Sample-Peak Limiter de latencia cero (0 muestras / 0.0 ms).
 * Por especificación acústica competitiva, no aplica filtros de sobremuestreo (True Peak / ISP)
 * para evitar el retardo algorítmico y la dispersión temporal que enmascararía disparos y pasos.
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

    /** Procesa un bloque mono en sitio. */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga estándar C++ con std::vector<float> (sin reasignaciones). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1 a 16 canales) con enlace de pico. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    /**
     * Evalúa el pico instantáneo multicanal y actualiza la ganancia lineal del limitador
     * para una muestra individual (usado en bucle por DynamicsProcessor).
     */
    [[nodiscard]] float computeSampleGainLinear(float peakAmplitude) noexcept;

    [[nodiscard]] float getCurrentGainLinear() const noexcept;
    [[nodiscard]] float getCurrentReductionDb() const noexcept;

    // Métricas de compatibilidad y arquitectura
    [[nodiscard]] bool isTruePeak() const noexcept { return false; }
    [[nodiscard]] int  getLatencySamples() const noexcept { return 0; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
