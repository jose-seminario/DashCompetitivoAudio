#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Parámetros del Limitador de Techo de Muestra Exacta (Sample-Peak Limiter).
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
 * Limitador de Techo de Muestra Exacta en Tiempo Real (LimiterProcessor).
 *
 * NOTA TÉCNICA Y DE ARQUITECTURA (ITU-R BS.1770-4):
 * Este procesador es un Limitador de Pico de Muestra (Sample-Peak Limiter) con ataque
 * instantáneo de cero latencia que garantiza matemáticamente que ninguna muestra digital
 * supere el umbral configurado (ceilingDb, -0.5 dBFS por defecto).
 *
 * ESPECIFICACIÓN PARA EVOLUCIÓN A TRUE PEAK (SOBREMUESTREO):
 *  - Para detectar picos inter-muestra (Inter-Sample Peaks) conforme al Anexo 2 de ITU-R BS.1770-4,
 *    se requiere un factor de sobremuestreo de 4x (o 2x a fs >= 88.2 kHz) mediante filtros FIR
 *    polifásicos semibanda (Half-Band Polyphase) y una ventana de anticipación (Lookahead) de
 *    al menos 0.5 a 1.0 ms (24 a 48 muestras a 48 kHz).
 *  - Dicho modo True Peak agregará una latencia determinista de N muestras, mientras que el
 *    modo actual opera con CERO latencia añadida, ideal para procesamiento táctico en juegos.
 *
 * Cero asignaciones de memoria dinámica dentro de process().
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
    [[nodiscard]] const LimiterParameters& getParameters() const noexcept;

    /** Procesa un bloque mono en sitio. */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga estándar C++ con std::vector<float> (sin reasignaciones). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1.0, 2.0, 5.1, 7.1) con enlace de pico. */
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
