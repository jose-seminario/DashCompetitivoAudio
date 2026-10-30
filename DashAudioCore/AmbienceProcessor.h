#pragma once

#include "AudioConstants.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Parámetros del Protector y Supresor Multibanda de Ruido Ambiental (AmbienceProcessor).
 */
struct AmbienceParameters
{
    // Parámetros tácticos canónicos (compatibilidad hacia atrás)
    float ambGuar = 1.0f;      // [0.0 .. 1.0] Factor global de atenuación selectiva de ruido
    bool  distant = true;      // [0 / 1] Protección de pistas distantes (pasos/voces)
    float target  = 0.388889f; // [0.0 .. 1.0] Umbral de activación (~ -24 dBFS RMS)
    float release = 0.0f;      // [0.0 .. 1.0] Relajación de ganancia del procesador
    float detRel  = 0.458333f; // [0.0 .. 1.0] Relajación del detector RMS (~295 ms)
    float scLow   = 0.2269f;   // [0.0 .. 1.0] Frecuencia inferior de sidechain (350.0 Hz)
    float scHigh  = 0.5293f;   // [0.0 .. 1.0] Frecuencia superior de sidechain (2500.0 Hz)
    bool  bandMod = true;      // [0 / 1] Modo multibanda activo (3 bandas Linkwitz-Riley)
    bool  bypass  = false;

    // Nuevos parámetros de ingeniería acústica avanzada
    float lowThresholdDb         = -26.0f; // [-60.0 .. -10.0 dB] Umbral de detección para banda baja (< 350 Hz)
    float midThresholdDb         = -24.0f; // [-60.0 .. -10.0 dB] Umbral para banda media (350 Hz - 2.5 kHz)
    float highThresholdDb        = -28.0f; // [-60.0 .. -10.0 dB] Umbral para banda alta (> 2.5 kHz)
    float lowReductionDepthDb    = -4.5f;  // [-18.0 .. 0.0 dB] Profundidad máxima de atenuación banda baja
    float midReductionDepthDb    = -6.0f;  // [-18.0 .. 0.0 dB] Profundidad máxima banda media
    float highReductionDepthDb   = -4.0f;  // [-18.0 .. 0.0 dB] Profundidad máxima banda alta
    float stationaritySensitivity= 0.50f;  // [0.0 .. 1.0] Sensibilidad de clasificación de estacionariedad
    float attackMs               = 25.0f;  // [5.0 .. 200.0 ms] Tiempo de ataque de reducción ambiental
    float releaseMs              = 180.0f; // [20.0 .. 1000.0 ms] Tiempo de relajación
    bool  bypassLowBand          = false;  // Bypass específico de banda baja
    bool  bypassMidBand          = false;  // Bypass específico de banda media
    bool  bypassHighBand         = false;  // Bypass específico de banda alta
    bool  useAdvancedMode        = false;  // true = usar parámetros avanzados; false = compatibilidad canónica
};

/**
 * Protector de Frecuencias Ambientales y Reductor de Ruido Estacionario (AmbienceProcessor).
 * Soporta de 1 a 16 canales simultáneos (kMaxSupportedChannels = 16).
 */
class AmbienceProcessor
{
public:
    static constexpr size_t kMaxChannels = kMaxSupportedChannels;

    AmbienceProcessor();
    ~AmbienceProcessor();

    AmbienceProcessor(AmbienceProcessor&&) noexcept;
    AmbienceProcessor& operator=(AmbienceProcessor&&) noexcept;

    AmbienceProcessor(const AmbienceProcessor&) = delete;
    AmbienceProcessor& operator=(const AmbienceProcessor&) = delete;

    [[nodiscard]] static float mapScNormToHz(float norm) noexcept;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;
    void updateParameters(const AmbienceParameters& params) noexcept;
    [[nodiscard]] const AmbienceParameters& getParameters() const noexcept;

    /** Procesa un bloque mono en sitio. */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1 a 16 canales) en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    [[nodiscard]] float getCurrentHpHz() const noexcept;
    [[nodiscard]] float getCurrentLpHz() const noexcept;
    [[nodiscard]] float getCurrentGainDb() const noexcept;
    [[nodiscard]] float getStationarityIndex() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
