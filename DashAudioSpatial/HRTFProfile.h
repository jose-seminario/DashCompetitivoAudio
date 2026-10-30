#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace dash::spatial
{

/**
 * Metadatos y Configuración Psicoacústica de un Perfil HRTF (Head-Related Transfer Function).
 */
struct HRTFProfile
{
    std::string id               = "default_tactical";
    std::string name             = "Dash Tactical HRTF Reference";
    std::string author           = "DashAudio";
    std::string license          = "AGPLv3 / Public Domain Acoustic Dataset";
    std::string description      = "Perfil binaural optimizado para localización horizontal y frontal.";

    double sampleRate            = 48000.0;
    size_t irLengthTaps          = 256;      // Longitud en muestras del filtro FIR (128 a 512)
    float  measurementDistanceM  = 1.4f;     // Distancia de medición del maniquí KEMAR en metros
    float  headRadiusM           = 0.0875f;  // Radio promedio de la cabeza humana (~8.75 cm)
    float  itdScaling            = 1.0f;     // Factor de ajuste para Diferencia Interaural de Tiempo (ITD)
    float  ildScaling            = 1.0f;     // Factor de ajuste para Diferencia Interaural de Nivel (ILD)
    bool   hasElevationFilters   = true;     // true si contiene filtros con discriminación de elevación

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] std::string getSummary() const;
};

} // namespace dash::spatial
