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
    std::wstring friendlyName;  // Ejemplo: "Hi-Fi Cable Input (VB-Audio Hi-Fi Cable)"
    std::wstring endpointGuid;  // Porción GUID "{xxxxxxxx-...}" para directivas "Device:" de Equalizer APO
    bool isDefaultConsole = false;
    bool isDefaultMultimedia = false;
};

/**
 * Administrador de políticas de audio y selección de endpoints en Windows.
 *
 * NOTA DE ARQUITECTURA Y SEGURIDAD:
 * La implementación interna utiliza la interfaz COM no documentada IPolicyConfig
 * (AudioSes.dll). Al ser una API privada del sistema operativo sujeta a posibles
 * modificaciones por parte de Microsoft, su definición se encuentra estrictamente
 * aislada en la unidad de traducción (AudioPolicyManager.cpp).
 *
 * Principios operativos:
 *  1. No expone tipos COM ni MIDL en encabezados públicos.
 *  2. No modifica el endpoint de audio predeterminado al inicializar el objeto.
 *  3. Solo conmutará dispositivos ante acciones explícitas del usuario (ej: click en "Activar Perfil").
 *  4. El llamador debe preservar el endpoint original para restaurarlo en deinit/cierre.
 */
class AudioPolicyManager
{
public:
    AudioPolicyManager() = default;
    ~AudioPolicyManager() = default;

    /**
     * Conmuta el dispositivo de reproducción predeterminado del sistema (roles eConsole y eMultimedia).
     *
     * @param deviceId ID completo de MMDevice (formato Windows).
     * @param outError Puntero opcional para recibir el detalle en caso de error.
     * @return true si la conmutación COM se realizó con éxito.
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
