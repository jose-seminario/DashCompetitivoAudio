#pragma once

#include "AudioDeviceInfo.h"
#include "AudioBackendState.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dash::audio
{

/**
 * Función callback para el procesamiento de bloques de audio en tiempo real.
 * Recibe los punteros a canales, cantidad de canales y cantidad de muestras.
 */
using AudioProcessCallback = std::function<void(float* const* channels, int numChannels, int numSamples)>;

/**
 * Función callback para notificaciones de cambio de estado o desconexión de dispositivos.
 */
using AudioStateCallback = std::function<void(AudioBackendState newState, const std::string& message)>;

/**
 * Interfaz abstracta pura para el backend de audio de la aplicación (IAudioBackend).
 *
 * Desacopla la lógica de control y DSP de las APIs concretas del sistema operativo
 * (WASAPI exclusivo, DirectSound, ASIO o hosting de Equalizer APO).
 */
class IAudioBackend
{
public:
    virtual ~IAudioBackend() = default;

    /** Inicializa subsistemas y bibliotecas del backend. */
    virtual bool initialize(std::string* outError = nullptr) = 0;

    /** Libera recursos y desconecta controladores. */
    virtual void shutdown() = 0;

    /** Enumera los dispositivos de reproducción (Render) disponibles. */
    virtual std::vector<AudioDeviceInfo> enumerateRenderDevices(std::string* outError = nullptr) = 0;

    /** Enumera los dispositivos de grabación o captura (Capture/Loopback) disponibles. */
    virtual std::vector<AudioDeviceInfo> enumerateCaptureDevices(std::string* outError = nullptr) = 0;

    /** Inicia el stream de audio con la configuración y callback indicados. */
    virtual bool start(
        const AudioBackendConfig& config,
        AudioProcessCallback processCallback,
        AudioStateCallback stateCallback,
        std::string* outError = nullptr) = 0;

    /** Detiene el stream de audio activo. */
    virtual bool stop(std::string* outError = nullptr) = 0;

    /** Devuelve el estado actual de ejecución del backend. */
    [[nodiscard]] virtual AudioBackendState getState() const noexcept = 0;

    /** Activa o desactiva el modo bypass del backend. */
    virtual void setBypass(bool bypassed) noexcept = 0;

    [[nodiscard]] virtual bool isBypassed() const noexcept = 0;

    /** Devuelve la última descripción de error reportada. */
    [[nodiscard]] virtual std::string getLastError() const = 0;
};

} // namespace dash::audio
