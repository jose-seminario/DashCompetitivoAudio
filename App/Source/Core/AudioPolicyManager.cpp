#include "AudioPolicyManager.h"

#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <wrl/client.h>
#include <functiondiscoverykeys_devpkey.h>

using Microsoft::WRL::ComPtr;
#endif

namespace dash::audio
{

namespace
{
#ifdef _WIN32
    // Guard RAII para inicializar COM de forma segura tanto en hilos nuevos
    // como en el Message Thread de JUCE (que ya inicializa COM en modo STA).
    class ScopedComInitializer
    {
    public:
        ScopedComInitializer() noexcept
        {
            hr_ = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            if (hr_ == RPC_E_CHANGED_MODE)
            {
                // El hilo actual ya tiene COM inicializado en MTA; es seguro operar sin desinicializar.
                shouldUninitialize_ = false;
                hr_ = S_OK;
            }
            else if (SUCCEEDED(hr_))
            {
                shouldUninitialize_ = true;
            }
        }

        ~ScopedComInitializer() noexcept
        {
            if (shouldUninitialize_)
                ::CoUninitialize();
        }

        ScopedComInitializer(const ScopedComInitializer&) = delete;
        ScopedComInitializer& operator=(const ScopedComInitializer&) = delete;

        bool succeeded() const noexcept { return SUCCEEDED(hr_); }
        HRESULT result() const noexcept { return hr_; }

    private:
        HRESULT hr_ = E_FAIL;
        bool shouldUninitialize_ = false;
    };

    std::string formatHResult(const char* context, HRESULT hr)
    {
        std::ostringstream oss;
        oss << context << " (HRESULT: 0x"
            << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
            << static_cast<unsigned long>(hr) << ")";
        return oss.str();
    }

    std::wstring getDefaultDeviceId(IMMDeviceEnumerator* enumerator, ERole role)
    {
        ComPtr<IMMDevice> defaultDevice;
        if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, role, defaultDevice.GetAddressOf())) || !defaultDevice)
            return {};

        LPWSTR rawId = nullptr;
        if (FAILED(defaultDevice->GetId(&rawId)) || rawId == nullptr)
            return {};

        std::wstring result(rawId);
        ::CoTaskMemFree(rawId);
        return result;
    }
#endif
} // namespace

bool AudioPolicyManager::setDefaultEndpoint(const std::wstring& deviceId, std::string* outError) const
{
    if (deviceId.empty())
    {
        if (outError) *outError = "El ID del dispositivo de audio está vacío.";
        return false;
    }

#ifdef _WIN32
    ScopedComInitializer comGuard;
    if (!comGuard.succeeded())
    {
        if (outError) *outError = formatHResult("Fallo al inicializar subsistema COM", comGuard.result());
        return false;
    }

    ComPtr<IPolicyConfig> policyConfig;
    HRESULT hr = ::CoCreateInstance(
        CLSID_PolicyConfigClient,
        nullptr,
        CLSCTX_ALL,
        __uuidof(IPolicyConfig),
        reinterpret_cast<void**>(policyConfig.GetAddressOf()));

    if (FAILED(hr) || !policyConfig)
    {
        if (outError) *outError = formatHResult("No se pudo instanciar CLSID_PolicyConfigClient", hr);
        return false;
    }

    // 1. Rol de Consola (Juegos, audio del sistema y aplicaciones interactivas)
    hr = policyConfig->SetDefaultEndpoint(deviceId.c_str(), eConsole);
    if (FAILED(hr))
    {
        if (outError) *outError = formatHResult("Fallo en SetDefaultEndpoint(eConsole)", hr);
        return false;
    }

    // 2. Rol Multimedia (Música, vídeo y motores de reproducción WASAPI multimedia)
    hr = policyConfig->SetDefaultEndpoint(deviceId.c_str(), eMultimedia);
    if (FAILED(hr))
    {
        if (outError) *outError = formatHResult("Fallo en SetDefaultEndpoint(eMultimedia)", hr);
        return false;
    }

    return true;
#else
    if (outError) *outError = "AudioPolicyManager requiere Windows.";
    return false;
#endif
}

std::vector<AudioEndpointInfo> AudioPolicyManager::enumerateActiveRenderEndpoints(std::string* outError) const
{
    std::vector<AudioEndpointInfo> endpoints;

#ifdef _WIN32
    ScopedComInitializer comGuard;
    if (!comGuard.succeeded())
    {
        if (outError) *outError = formatHResult("Fallo al inicializar COM para enumeración", comGuard.result());
        return endpoints;
    }

    ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = ::CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(enumerator.GetAddressOf()));

    if (FAILED(hr) || !enumerator)
    {
        if (outError) *outError = formatHResult("Fallo al crear IMMDeviceEnumerator", hr);
        return endpoints;
    }

    const std::wstring defaultConsoleId = getDefaultDeviceId(enumerator.Get(), eConsole);
    const std::wstring defaultMultimediaId = getDefaultDeviceId(enumerator.Get(), eMultimedia);

    ComPtr<IMMDeviceCollection> collection;
    hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, collection.GetAddressOf());
    if (FAILED(hr) || !collection)
    {
        if (outError) *outError = formatHResult("Fallo en EnumAudioEndpoints", hr);
        return endpoints;
    }

    UINT count = 0;
    collection->GetCount(&count);
    endpoints.reserve(count);

    for (UINT i = 0; i < count; ++i)
    {
        ComPtr<IMMDevice> device;
        if (FAILED(collection->Item(i, device.GetAddressOf())) || !device)
            continue;

        LPWSTR rawId = nullptr;
        if (FAILED(device->GetId(&rawId)) || rawId == nullptr)
            continue;

        AudioEndpointInfo info;
        info.id.assign(rawId);
        ::CoTaskMemFree(rawId);

        info.endpointGuid = extractEndpointGuid(info.id);
        info.isDefaultConsole = (info.id == defaultConsoleId);
        info.isDefaultMultimedia = (info.id == defaultMultimediaId);

        ComPtr<IPropertyStore> propStore;
        if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, propStore.GetAddressOf())) && propStore)
        {
            PROPVARIANT varName;
            ::PropVariantInit(&varName);
            if (SUCCEEDED(propStore->GetValue(PKEY_Device_FriendlyName, &varName)) && varName.vt == VT_LPWSTR && varName.pwszVal != nullptr)
            {
                info.friendlyName.assign(varName.pwszVal);
            }
            ::PropVariantClear(&varName);
        }

        if (info.friendlyName.empty())
            info.friendlyName = info.id;

        endpoints.push_back(std::move(info));
    }
#else
    if (outError) *outError = "Enumeración MMDevice requiere Windows.";
#endif

    return endpoints;
}

std::wstring AudioPolicyManager::extractEndpointGuid(const std::wstring& fullDeviceId)
{
    // Formato estándar MMDevice: "{0.0.0.00000000}.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
    const auto lastOpenBrace = fullDeviceId.rfind(L'{');
    if (lastOpenBrace != std::wstring::npos)
        return fullDeviceId.substr(lastOpenBrace);

    return fullDeviceId;
}

} // namespace dash::audio
