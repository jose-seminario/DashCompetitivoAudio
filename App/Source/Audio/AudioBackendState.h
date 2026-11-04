#pragma once

#include <string>

namespace dash::audio
{

/**
 * Estados del ciclo de vida del backend de audio en tiempo de ejecución.
 */
enum class AudioBackendState
{
    Uninitialized,      // Backend no configurado
    Stopped,            // Inicializado pero detenido (sin flujo de audio activo)
    Running,            // Procesando audio estéreo activamente
    Bypassed,           // Corriendo en modo bypass (traspaso directo sin DSP)
    DeviceDisconnected, // El dispositivo de audio fue desconectado físicamente
    Error               // Fallo crítico de stream o formato
};

/**
 * Parámetros de inicialización y formato para el stream de audio estéreo.
 */
struct AudioBackendConfig
{
    std::wstring deviceId;             // ID del dispositivo destino (reproducción / render)
    std::wstring inputDeviceId;        // ID opcional del dispositivo de entrada/captura
    double       sampleRate       = 48000.0;
    int          bufferSize       = 512;
    int          numChannels      = 2;
    int          numInputChannels = 2;
    bool         exclusiveMode    = false;
};

} // namespace dash::audio
