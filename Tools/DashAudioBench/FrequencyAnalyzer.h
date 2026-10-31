#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace dash::bench
{

struct SpectralBandsEnergy
{
    float subBassDb   = -100.0f; // 20 - 80 Hz
    float bassDb      = -100.0f; // 80 - 250 Hz
    float lowMidsDb   = -100.0f; // 250 - 800 Hz
    float midsDb      = -100.0f; // 800 - 2500 Hz
    float footstepsDb = -100.0f; // 2500 - 6500 Hz (Banda táctica clave)
    float gunCrackDb  = -100.0f; // 2000 - 6000 Hz (Transitorio de disparos / Gun Crack)
    float highsDb     = -100.0f; // 6500 - 20000 Hz
};

/**
 * Analizador Espectral y de Respuesta en Frecuencia (FrequencyAnalyzer).
 *
 * Mide la energía acústica en bandas tácticas críticas para verificar
 * la inteligibilidad de pasos y la atenuación de disparos.
 */
class FrequencyAnalyzer
{
public:
    static SpectralBandsEnergy analyzeBands(const float* buffer, size_t numSamples, double sampleRate) noexcept;
    static std::vector<float> generateSineSweep(double sampleRate, size_t numSamples, float startFreq = 20.0f, float endFreq = 20000.0f);
};

} // namespace dash::bench
