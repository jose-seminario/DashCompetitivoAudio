#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

struct DynamicsParameters
{
    float amount      = 0.75f;     // [0.0 .. 1.0] Intensidad global híbrida (Upward + Downward)
    float targetLufs  = -18.0f;    // [-28.0 .. +8.0] Objetivo ITU-R BS.1770-4 (-18 LUFS por defecto)
    float releaseNorm = 0.50f;     // [0.0 .. 1.0] Relajación de ganancia del compresor
    float detRelNorm  = 0.583333f; // [0.0 .. 1.0] Relajación del detector RMS (~400 ms BS.1770 Momentary)
    float limRelNorm  = 0.612903f; // [0.0 .. 1.0] Relajación del limitador de picos de salida
    float scLowNorm   = 0.0f;      // [0.0 .. 1.0] Frecuencia de corte inferior del sidechain
    float scHighNorm  = 1.0f;      // [0.0 .. 1.0] Frecuencia de corte superior del sidechain
    float maxLiftDb   = 9.0f;      // [0.0 .. 18.0] Ganancia ascendente máxima (Upward Lift para pasos)
    float gateFloorDb = -62.0f;    // [-80.0 .. -35.0] Umbral inferior anti-ruido de fondo
    bool  bypass      = false;
};

/**
 * Motor DSP central de Compresión Dinámica Híbrida (Upward + Downward)
 * gobernado por detección perceptual ITU-R BS.1770-4 (ponderación K) y Limitador de Techo.
 *
 * API pública C++20 pura sin dependencias de tipos de JUCE, VST o interfaz gráfica.
 * Garantiza cero asignaciones dinámicas dentro de process().
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

    /** Procesa un bloque mono en sitio (API estándar C++). */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1.0, 2.0, 5.1, 7.1) en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    [[nodiscard]] float getCurrentLufs() const noexcept;
    [[nodiscard]] float getCurrentGainDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
