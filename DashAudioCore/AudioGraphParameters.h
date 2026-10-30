#pragma once

#include "DynamicsProcessor.h"
#include "TransientProcessor.h"
#include "AmbienceProcessor.h"
#include "EQProcessor.h"
#include "ParametricEQProcessor.h"
#include "LimiterProcessor.h"

#include <array>

namespace dash::core
{

/**
 * Parámetros integrados de configuración de la cadena completa de procesamiento (AudioGraph).
 */
struct AudioGraphParameters
{
    float headroomDb = -6.0f; // Ganancia inicial de margen dinámico para evitar clipping inter-etapa

    DynamicsParameters  dynamics {};   // 1. Compresor dinámico híbrido ITU-R BS.1770-4
    TransientParameters transient {};  // 2. Supresor de transitorios de disparos (Gun Crack)
    AmbienceParameters  ambience {};   // 3. Protector ambiental multibanda (350 Hz - 2.5 kHz)
    EQParameters        equalizer {};  // 4. Ecualización acústica de 31 bandas ISO
    LimiterParameters   limiter {};    // 5. Limitador final de seguridad (-0.5 dBFS)

    // Módulo opcional de ecualización paramétrica de precisión
    bool useParametricEq = false;
    std::array<ParametricBandParameters, ParametricEQProcessor::kMaxBands> parametricBands {};

    bool bypassAll = false; // Bypass global de la cadena
};

} // namespace dash::core
