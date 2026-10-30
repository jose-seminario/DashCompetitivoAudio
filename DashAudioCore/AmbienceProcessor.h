#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

struct AmbienceParameters
{
    float ambGuar = 1.0f;      // [0.0 .. 1.0] Atenuación selectiva de ruido ambiental estacionario
    bool  distant = true;      // [0 / 1] Protege pistas dinámicas lejanas (pasos/voces) frente al piso ambiental
    float target  = 0.388889f; // [0.0 .. 1.0] Umbral base de nuestro DSP (-14 LUFS / -24 dBFS RMS normalizado)
    float release = 0.0f;      // [0.0 .. 1.0] Relajación de ganancia del compresor
    float detRel  = 0.458333f; // [0.0 .. 1.0] Relajación del detector RMS (~295 ms)
    float scLow   = 0.2269f;   // [0.0 .. 1.0] 0.2269 => 350.0 Hz exactos (High-Pass IIR Sidechain)
    float scHigh  = 0.5293f;   // [0.0 .. 1.0] 0.5293 => 2500.0 Hz (2.5 kHz) exactos (Low-Pass IIR Sidechain)
    bool  bandMod = true;      // [0 / 1] Procesamiento Multibanda de 3 bandas (1 = activo por defecto)
    bool  bypass  = false;
};

/**
 * Compresor Multibanda y Protector de Frecuencias Ambientales (AmbienceProcessor).
 *
 * Restringe el detector Sidechain mediante filtros biquad IIR (350 Hz - 2.5 kHz por defecto)
 * y divide el espectro en 3 bandas Linkwitz-Riley alineadas en fase.
 *
 * API pública C++20 estándar libre de dependencias de JUCE o VST.
 * Cero asignaciones dinámicas dentro de process() y updateParameters().
 */
class AmbienceProcessor
{
public:
    static constexpr size_t kMaxChannels = 8;

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

    /** Procesa un bloque mono en sitio (API estándar C++). */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1.0, 2.0, 5.1, 7.1) en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    [[nodiscard]] float getCurrentHpHz() const noexcept;
    [[nodiscard]] float getCurrentLpHz() const noexcept;
    [[nodiscard]] float getCurrentGainDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
