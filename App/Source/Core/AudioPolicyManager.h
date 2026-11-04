#pragma once

#include <string>
#include <vector>

namespace dash::audio
{

/**
 * Información de un Endpoint MMDevice de Windows.
 */
struct AudioEndpointInfo
{
    std::wstring id;            // Ejemplo: "{0.0.0.00000000}.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
    std::wstring friendlyName;  // Ejemplo: "Speakers (Realtek High Definition Audio)"
    std::wstring endpointGuid;  // Porción GUID "{xxxxxxxx-...}" para directivas "Device:" de Equalizer APO
    bool isDefaultConsole = false;
    bool isDefaultMultimedia = false;
};

/**
 * Administrador de políticas de audio y selección de endpoints estéreo en Windows.
 *
 * NOTA DE ARQUITECTURA Y SEGURIDAD:
 * La implementación interna utiliza la interfaz COM IPolicyConfig (AudioSes.dll),
 * estrictamente aislada dentro de la unidad de traducción (AudioPolicyManager.cpp).
 *
 * Principios operativos:
 *  1. No expone tipos COM ni MIDL en encabezados públicos.
 *  2. No modifica el endpoint de audio predeterminado al inicializar el objeto.
 *  3. Solo conmutará dispositivos ante selección explícita del usuario en Ajustes.
 */
class AudioPolicyManager
{
public:
    AudioPolicyManager() = default;
    ~AudioPolicyManager() = default;

    /**
     * Conmuta el dispositivo de reproducción predeterminado del sistema (roles eConsole y eMultimedia).
     */
    bool setDefaultEndpoint(const std::wstring& deviceId, std::string* outError = nullptr) const;

    /**
     * Enumera todos los dispositivos de salida (Render) activos en el subsistema Core Audio de Windows.
     */
    std::vector<AudioEndpointInfo> enumerateActiveRenderEndpoints(std::string* outError = nullptr) const;

    /**
     * Extrae el token "{GUID}" final de un ID MMDevice completo para directivas APO.
     */
    static std::wstring extractEndpointGuid(const std::wstring& fullDeviceId);
};

} // namespace dash::audio
