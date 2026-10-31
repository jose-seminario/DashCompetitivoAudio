#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace dash::spatial
{

/**
 * Metadatos y Configuración Psicoacústica de un Perfil HRTF basado en el Modelo Esférico Woodworth.
 *
 * NOTA DE RIGOR CIENTÍFICO Y ACÚSTICO:
 * Este modelo implementa la aproximación analítica clásica esférica de Woodworth con escalado
 * paramétrico de cabeza (Woodworth Spherical Model with Parametric Head Scaling).
 * No constituye un HRTF anatómico individualizado de oreja humana (escaneo 3D / SOFA), sino una
 * parametrización geométrica de radio de cabeza y escalas ITD/ILD. La discriminación vertical
 * se implementa como "Approximated Pinna Notch Filtering".
 */
struct HRTFProfile
{
    std::string id               = "default_tactical";
    std::string name             = "Dash Woodworth Spherical Model";
    std::string author           = "DashAudio Acoustics";
    std::string license          = "AGPLv3 / Public Domain Acoustic Dataset";
    std::string description      = "Modelo analítico esférico Woodworth con escalado paramétrico de cabeza y filtrado de elevación aproximado.";

    double sampleRate            = 48000.0;
    size_t irLengthTaps          = 256;      // Longitud en muestras del filtro FIR (128 a 512)
    float  measurementDistanceM  = 1.4f;     // Distancia de medición de referencia en metros
    float  headRadiusM           = 0.0875f;  // Radio promedio de la cabeza humana (~8.75 cm)
    float  itdScaling            = 1.0f;     // Factor de ajuste para Diferencia Interaural de Tiempo (ITD)
    float  ildScaling            = 1.0f;     // Factor de ajuste para Diferencia Interaural de Nivel (ILD)
    bool   hasElevationFilters   = true;     // true si aplica Approximated Pinna Notch Filtering

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] std::string getSummary() const;
};

} // namespace dash::spatial
