#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Parámetros del Compresor Dinámico Híbrido ITU-R BS.1770-4.
 *
 * Mantiene compatibilidad total con la API previa mediante constructores
 * y asignaciones normalizadas, agregando controles acústicos avanzados en milisegundos,
 * decibeles y ratios de compresión.
 */
struct DynamicsParameters
{
    // Parámetros canónicos de la línea base táctica (compatibilidad hacia atrás)
    float amount      = 0.75f;     // [0.0 .. 1.0] Intensidad global híbrida
    float targetLufs  = -18.0f;    // [-28.0 .. +8.0 LUFS] Objetivo perceptual ITU-R BS.1770-4
    float releaseNorm = 0.50f;     // [0.0 .. 1.0] Relajación normalizada de ganancia
    float detRelNorm  = 0.583333f; // [0.0 .. 1.0] Relajación del detector RMS (~400 ms)
    float limRelNorm  = 0.612903f; // [0.0 .. 1.0] Relajación del limitador de seguridad
    float scLowNorm   = 0.0f;      // [0.0 .. 1.0] Corte inferior normalizado del sidechain
    float scHighNorm  = 1.0f;      // [0.0 .. 1.0] Corte superior normalizado del sidechain
    float maxLiftDb   = 9.0f;      // [0.0 .. 18.0 dB] Ganancia ascendente máxima (Upward Lift)
    float gateFloorDb = -62.0f;    // [-80.0 .. -35.0 dB] Umbral inferior de la compuerta anti-ruido
    bool  bypass      = false;

    // Nuevos parámetros de ingeniería acústica avanzada
    float thresholdDb      = -18.0f;  // Umbral acústico en dBFS/LUFS
    float ratio            = 3.5f;    // [1.0 .. 20.0] Ratio de compresión descendente (Downward)
    float kneeWidthDb      = 6.0f;    // [0.0 .. 18.0 dB] Ancho de la rodilla suave (Soft Knee)
    float attackMs         = 12.0f;   // [0.1 .. 100.0 ms] Tiempo de ataque de ganancia descendente
    float releaseMs        = 120.0f;  // [5.0 .. 1000.0 ms] Tiempo de relajación de ganancia descendente
    float upwardAmount     = 0.75f;   // [0.0 .. 1.0] Intensidad de elevación de micro-detalles (Upward)
    float downwardAmount   = 0.75f;   // [0.0 .. 1.0] Intensidad de atenuación de disparos/picos (Downward)
    float makeupGainDb     = 0.0f;    // [-12.0 .. +12.0 dB] Compensación final de ganancia
    float scLowHz          = 20.0f;   // [20.0 .. 1000.0 Hz] Frecuencia de corte pasa-altos de sidechain
    float scHighHz         = 20000.0f;// [1000.0 .. 20000.0 Hz] Frecuencia de corte pasa-bajos de sidechain
    float gateThresholdDb  = -62.0f;  // [-90.0 .. -30.0 dB] Umbral de corte de la compuerta de ruido
    bool  useAdvancedMode  = false;   // true = usar parámetros en ms/dB; false = compatibilidad táctica normalizada
};

/**
 * Motor DSP de Compresión Dinámica Híbrida (Upward + Downward).
 *
 * Características matemáticas:
 *  - Detección perceptual mediante filtro de ponderación K (ITU-R BS.1770-4):
 *      Etapa 1 (Pre-filter): Filtro High-Shelf @ 1681.97 Hz, +3.9998 dB, Q = 0.7071
 *      Etapa 2 (RLB filter): Filtro High-Pass @ 38.135 Hz, Q = 0.5003
 *  - Ponderación multicanal espacial:
 *      Canales Izquierdo, Derecho, Central (L, R, C): G_i = 1.0 (0 dB)
 *      Canal LFE (Subwoofer): G_LFE = 0.0 (estrictamente excluido de la medición de sonoridad)
 *      Canales Surround (Ls, Rs, Lrs, Rrs): G_s = 1.41 (~ +1.5 dB)
 *  - Conversión a LUFS estandarizada:
 *      L_K = -0.691 + 10 * log10( sum_i( G_i * z_i ) )
 *  - Curva de compresión dual con rodilla suave cuadrática (Soft-Knee):
 *      Downward: Atenúa picos y explosiones por encima del umbral.
 *      Upward: Eleva pasos y recargas por debajo del umbral sin amplificar el piso de ruido.
 *  - Telemetría en tiempo real no bloqueante.
 *  - Cero asignaciones de memoria dinámica dentro de process().
 */
class DynamicsProcessor
{
public:
    static constexpr size_t kMaxChannels = 8;

    DynamicsProcessor();
    ~DynamicsProcessor();

    DynamicsProcessor(DynamicsProcessor&&) noexcept;
    DynamicsProcessor& operator=(DynamicsProcessor&&) noexcept;

    DynamicsProcessor(const DynamicsProcessor&) = delete;
    DynamicsProcessor& operator=(const DynamicsProcessor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;
    void updateParameters(const DynamicsParameters& params) noexcept;
    [[nodiscard]] const DynamicsParameters& getParameters() const noexcept;

    /** Procesa un bloque mono en sitio (API estándar C++). */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1.0, 2.0, 5.1, 7.1) en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    // Métricas y telemetría en tiempo real
    [[nodiscard]] float getCurrentLufs() const noexcept;
    [[nodiscard]] float getCurrentGainDb() const noexcept;
    [[nodiscard]] float getCurrentPeakDb() const noexcept;
    [[nodiscard]] float getUpwardGainDb() const noexcept;
    [[nodiscard]] float getDownwardGainDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
