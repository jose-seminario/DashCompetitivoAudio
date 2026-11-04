#pragma once

#include "AudioConstants.h"

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
    float gainDb               = 0.0f;    // [-12.0 .. +12.0 dB]
    float q                    = 0.707f;  // [0.1 .. 18.0] Factor de calidad
    ParametricFilterType type  = ParametricFilterType::Peaking;
    bool  enabled              = true;
};

/**
 * Ecualizador Táctico Estéreo de 6 Bandas (ParametricEQProcessor).
 *
 * Bandas configuradas por defecto:
 *  - Banda 0 (Sub-Bajos):    LowShelf  60 Hz   (Q = 0.707)
 *  - Banda 1 (Bajos):        LowShelf  150 Hz  (Q = 0.707)
 *  - Banda 2 (Cuerpo):       Peaking   400 Hz  (Q = 1.0)
 *  - Banda 3 (Pasos Medios): Peaking   1500 Hz (Q = 1.2)
 *  - Banda 4 (Pasos Agudos): Peaking   3500 Hz (Q = 1.2)
 *  - Banda 5 (Disparos):     HighShelf 8000 Hz (Q = 0.707)
 */
class ParametricEQProcessor
{
public:
    static constexpr size_t kMaxBands    = 6;
    static constexpr size_t kMaxChannels = kMaxSupportedChannels;

    static constexpr std::array<const char*, kMaxBands> kBandNames {
        "Sub-Bajos",
        "Bajos",
        "Cuerpo",
        "Pasos Medios",
        "Pasos Agudos",
        "Disparos"
    };

    [[nodiscard]] static constexpr std::array<ParametricBandParameters, kMaxBands> getDefaultBands() noexcept
    {
        return {{
            { 60.0f,   0.0f, 0.707f, ParametricFilterType::LowShelf,  true },
            { 150.0f,  0.0f, 0.707f, ParametricFilterType::LowShelf,  true },
            { 400.0f,  0.0f, 1.0f,   ParametricFilterType::Peaking,   true },
            { 1500.0f, 0.0f, 1.2f,   ParametricFilterType::Peaking,   true },
            { 3500.0f, 0.0f, 1.2f,   ParametricFilterType::Peaking,   true },
            { 8000.0f, 0.0f, 0.707f, ParametricFilterType::HighShelf, true }
        }};
    }

    ParametricEQProcessor();
    ~ParametricEQProcessor();

    ParametricEQProcessor(ParametricEQProcessor&&) noexcept;
    ParametricEQProcessor& operator=(ParametricEQProcessor&&) noexcept;

    ParametricEQProcessor(const ParametricEQProcessor&) = delete;
    ParametricEQProcessor& operator=(const ParametricEQProcessor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;
    void resetBandsToDefaults() noexcept;

    void setBand(size_t bandIndex, const ParametricBandParameters& params) noexcept;
    void setBandGainDb(size_t bandIndex, float gainDb) noexcept;
    [[nodiscard]] ParametricBandParameters getBand(size_t bandIndex) const noexcept;

    void setBypass(bool bypass) noexcept;
    [[nodiscard]] bool isBypassed() const noexcept;

    /** Procesa las 6 bandas tácticas estáticas en estéreo (1 o 2 canales) sin asignaciones dinámicas. */
    void process(float* buffer, int numSamples) noexcept;
    void process(std::vector<float>& buffer) noexcept;
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
