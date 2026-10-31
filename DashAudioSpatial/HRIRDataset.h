#pragma once

#include "ChannelLayout.h"
#include "HRTFProfile.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace dash::spatial
{

/**
 * Par de Respuestas al Impulso Binaural (Oído Izquierdo y Oído Derecho) para un altavoz.
 */
struct HRIRPair
{
    std::vector<float> leftEar;
    std::vector<float> rightEar;
    float peakGain = 1.0f;
    float delaySamples = 0.0f;

    [[nodiscard]] bool isValid() const noexcept
    {
        return !leftEar.empty() && leftEar.size() == rightEar.size();
    }
};

/**
 * Gestor de Conjuntos de Datos HRIR (Head-Related Impulse Response).
 *
 * Principios:
 *  - Carga y procesamiento fuera del hilo de audio.
 *  - Normalización acústica para preservar la energía espectral RMS.
 *  - Generador de referencia sintético Woodworth (libre de licencias de terceros)
 *    basado en el modelo esférico clásico de difracción de la cabeza humana:
 *      ITD(theta) = (r / c) * (theta + sin(theta))
 *  - Soporte de almacenamiento de hasta kMaxChannels pares para layouts inmersivos.
 */
class HRIRDataset
{
public:
    static constexpr size_t kMaxChannels = ChannelLayout::kMaxChannels;

    HRIRDataset();
    ~HRIRDataset();

    HRIRDataset(HRIRDataset&&) noexcept;
    HRIRDataset& operator=(HRIRDataset&&) noexcept;

    HRIRDataset(const HRIRDataset&) = delete;
    HRIRDataset& operator=(const HRIRDataset&) = delete;

    /**
     * Genera un conjunto sintético analítico basado en el modelo esférico KEMAR / Woodworth.
     */
    bool loadSyntheticReference(
        const ChannelLayout& layout,
        double sampleRate = 48000.0,
        size_t irLength = 256);

    /**
     * Asigna manualmente un par de respuestas al impulso para un canal específico.
     *
     * ADVERTENCIA DE FRECUENCIA DE MUESTREO (SAMPLE RATE):
     * El motor de convolución directa opera a la frecuencia de muestreo activa sin
     * resampleador polifásico interno. La inyección de respuestas al impulso externas
     * exige que el emisor suministre los coeficientes exactamente a la misma frecuencia de
     * muestreo que la sesión activa (o realizar resampling sinc previo) para evitar
     * distorsiones de tono y desajustes temporales en los retardos ITD.
     */
    bool setImpulsePair(
        size_t channelIndex,
        const float* leftTaps,
        const float* rightTaps,
        size_t numTaps);

    [[nodiscard]] const HRIRPair* getImpulsePair(size_t channelIndex) const noexcept;
    [[nodiscard]] const HRTFProfile& getProfile() const noexcept { return profile_; }
    void setProfile(const HRTFProfile& profile) noexcept { profile_ = profile; }

    [[nodiscard]] bool isLoaded() const noexcept { return isLoaded_; }
    [[nodiscard]] size_t getIRLength() const noexcept { return irLength_; }
    [[nodiscard]] double getSampleRate() const noexcept { return profile_.sampleRate; }

private:
    HRTFProfile profile_;
    size_t irLength_ = 256;
    bool isLoaded_   = false;
    std::array<HRIRPair, kMaxChannels> pairs_ {};
};

} // namespace dash::spatial
