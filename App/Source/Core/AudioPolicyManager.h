#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <mmreg.h>
#include <mmdeviceapi.h>
#include <propsys.h>

// ==============================================================================
// Definición binaria exacta de la interfaz COM no documentada IPolicyConfig
// Presente en AudioSes.dll (Windows Vista / 7 / 10 / 11)
// CLSID: {870AF99C-171D-4F9E-AF0D-E63DF40C2BC9}
// IID:   {F8679F50-850A-41CF-9C72-430F290290C8}
// ==============================================================================

struct DeviceShareMode;

// {870AF99C-171D-4F9E-AF0D-E63DF40C2BC9}
inline constexpr GUID CLSID_PolicyConfigClient = {
    0x870af99c, 0x171d, 0x4f9e, { 0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9 }
};

// {F8679F50-850A-41CF-9C72-430F290290C8}
MIDL_INTERFACE("f8679f50-850a-41cf-9c72-430f290290c8")
IPolicyConfig : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(
        PCWSTR pszDeviceName,
        WAVEFORMATEX** ppFormat) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(
        PCWSTR pszDeviceName,
        INT bDefault,
        WAVEFORMATEX** ppFormat) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(
        PCWSTR pszDeviceName) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(
        PCWSTR pszDeviceName,
        WAVEFORMATEX* pEndpointFormat,
        WAVEFORMATEX* pMixFormat) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(
        PCWSTR pszDeviceName,
        INT bDefault,
        PINT64 pmftDefaultPeriod,
        PINT64 pmftMinimumPeriod) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(
        PCWSTR pszDeviceName,
        PINT64 pmftPeriod) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetShareMode(
        PCWSTR pszDeviceName,
        DeviceShareMode* pMode) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetShareMode(
        PCWSTR pszDeviceName,
        DeviceShareMode* pMode) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
        PCWSTR pszDeviceName,
        BOOL bFxStore,
        const PROPERTYKEY& key,
        PROPVARIANT* pv) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(
        PCWSTR pszDeviceName,
        BOOL bFxStore,
        const PROPERTYKEY& key,
        const PROPVARIANT& pv) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(
        PCWSTR wszDeviceId,
        ERole role) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(
        PCWSTR wszDeviceId,
        INT bVisible) = 0;
};
#endif

namespace dash::audio
{

struct AudioEndpointInfo
{
    std::wstring id;            // Ejemplo: "{0.0.0.00000000}.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
    std::wstring friendlyName;  // Ejemplo: "Hi-Fi Cable Input (VB-Audio Hi-Fi Cable)"
    std::wstring endpointGuid;  // Solo la porción GUID "{xxxxxxxx-...}" usada por Equalizer APO
    bool isDefaultConsole = false;
    bool isDefaultMultimedia = false;
};

class AudioPolicyManager
{
public:
    AudioPolicyManager() = default;
    ~AudioPolicyManager() = default;

    /**
     * Cambia el dispositivo de reproducción predeterminado del sistema invocando
     * directamente IPolicyConfig::SetDefaultEndpoint para los roles de Consola (Juegos/Sistema)
     * y Multimedia.
     *
     * @param deviceId ID completo del endpoint MMDevice de Windows.
     * @param outError Mensaje descriptivo en caso de fallo (opcional).
     * @return true si ambos roles se conmutaron con éxito.
     */
    bool setDefaultEndpoint(const std::wstring& deviceId, std::string* outError = nullptr) const;

    /**
     * Enumera todos los dispositivos de salida (Render) activos en Windows.
     */
    std::vector<AudioEndpointInfo> enumerateActiveRenderEndpoints(std::string* outError = nullptr) const;

    /**
     * Extrae la porción "{GUID}" final de un MMDevice ID completo "{0.0.0.00000000}.{GUID}"
     * para usarla en directivas "Device: {GUID}" de Equalizer APO.
     */
    static std::wstring extractEndpointGuid(const std::wstring& fullDeviceId);
};

} // namespace dash::audio
