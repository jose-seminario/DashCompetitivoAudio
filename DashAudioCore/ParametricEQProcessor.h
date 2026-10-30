#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Tipos de filtro acústico de segundo orden (Biquad IIR).
 */
enum class ParametricFilterType
{
    Peaking,   // Campana paramétrica (Bell / Peak)
    LowShelf,  // Estante de graves
    HighShelf, // Estante de agudos
    HighPass,  // Pasa-altos de 12 dB/oct (segundo orden)
    LowPass,   // Pasa-bajos de 12 dB/oct (segundo orden)
    Notch      // Filtro trampa de banda estrecha
};

/**
 * Configuración de una banda paramétrica individual.
 */
struct ParametricBandParameters
{
    float frequencyHz          = 1000.0f; // [20.0 .. 20000.0 Hz]
    float gainDb               = 0.0f;    // [-24.0 .. +24.0 dB] (solo para Peaking y Shelves)
    float q                    = 0.7071f; // [0.1 .. 18.0] Factor de calidad / Resonancia
    ParametricFilterType type  = ParametricFilterType::Peaking;
    bool  enabled              = true;
};

/**
 * Ecualizador Paramétrico Multicanal de Precisión (ParametricEQProcessor).
 *
 * Características:
 *  - Soporta hasta kMaxBands (10) bandas por canal totalmente configurables.
 *  - Implementación biquad en Forma Directa II Transpuesta (DF2T) numéricamente
 *    estable frente a errores de redondeo de punto flotante en bajas frecuencias.
 *  - Interpolación suave de coeficientes para eliminar chasquidos (zipper noise)
 *    al automatizar ganancias, frecuencias o resonancias.
 *  - Multicanal hasta kMaxChannels (8) canales: mono, estéreo, 5.1 y 7.1.
 *  - Cero asignaciones de memoria dinámica dentro de process().
 */
class ParametricEQProcessor
{
public:
    static constexpr size_t kMaxBands    = 10;
    static constexpr size_t kMaxChannels = 8;

    ParametricEQProcessor();
    ~ParametricEQProcessor();

    ParametricEQProcessor(ParametricEQProcessor&&) noexcept;
    ParametricEQProcessor& operator=(ParametricEQProcessor&&) noexcept;

    ParametricEQProcessor(const ParametricEQProcessor&) = delete;
    ParametricEQProcessor& operator=(const ParametricEQProcessor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;

    void setBand(size_t bandIndex, const ParametricBandParameters& params) noexcept;
    [[nodiscard]] ParametricBandParameters getBand(size_t bandIndex) const noexcept;

    void setBypass(bool bypass) noexcept;
    [[nodiscard]] bool isBypassed() const noexcept;

    /** Procesa un bloque mono en sitio. */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
