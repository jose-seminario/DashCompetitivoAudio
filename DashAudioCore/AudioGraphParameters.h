#pragma once

#include "ParametricEQProcessor.h"
#include "LimiterProcessor.h"

#include <array>

namespace dash::core
{

/**
 * Parámetros de la cadena estéreo táctica de 6 bandas (AudioGraph).
 * Cadena: Entrada Estéreo (2ch) -> Preamp -> ParametricEQ (6 bandas) -> Limiter (-0.5 dBFS) -> Salida.
 */
struct AudioGraphParameters
{
    float preampDb = 0.0f; // Ganancia previa [-18.0 .. +18.0 dB]

    // Ganancias en dB de las 6 bandas tácticas [-18.0 .. +18.0 dB]:
    //  [0] Sub-Bajos    (60 Hz,   LowShelf,  Q = 0.707)
    //  [1] Bajos        (150 Hz,  LowShelf,  Q = 0.707)
    //  [2] Cuerpo       (400 Hz,  Peaking,   Q = 1.0)
    //  [3] Pasos Medios (1500 Hz, Peaking,   Q = 1.2)
    //  [4] Pasos Agudos (3500 Hz, Peaking,   Q = 1.2)
    //  [5] Disparos     (8000 Hz, HighShelf, Q = 0.707)
    std::array<float, ParametricEQProcessor::kMaxBands> bandGainsDb {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };

    LimiterParameters limiter {}; // Limitador Sample-Peak final (-0.5 dBFS)
    bool bypassAll = false;       // Bypass global de la cadena
};

} // namespace dash::core
