#include "SystemDependencyChecker.h"

#include <algorithm>
#include <cwctype>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace dash::audio
{

namespace
{
    std::wstring toLowerWide(std::wstring s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        return s;
    }

#ifdef _WIN32
    bool checkProcessElevated() noexcept
    {
        HANDLE hToken = nullptr;
        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
            return false;

        TOKEN_ELEVATION elevation {};
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        const BOOL ok = ::GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize);
        ::CloseHandle(hToken);

        return (ok != FALSE) && (elevation.TokenIsElevated != 0);
    }

    std::filesystem::path queryRegistryApoInstallDir()
    {
        HKEY hKey = nullptr;
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\EqualizerAPO", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
            return {};

        wchar_t buffer[MAX_PATH] {};
        DWORD bufferBytes = sizeof(buffer);
        DWORD type = 0;
        const LONG res = ::RegQueryValueExW(hKey, L"InstallPath", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferBytes);
        ::RegCloseKey(hKey);

        if (res == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ) && buffer[0] != L'\0')
            return std::filesystem::path(buffer);

        return {};
    }

    bool checkVbDriverInRegistryOrDisk()
    {
        std::error_code ec;
        if (std::filesystem::exists("C:\\Program Files\\VB\\CABLE", ec)
            || std::filesystem::exists("C:\\Program Files (x86)\\VB\\CABLE", ec)
            || std::filesystem::exists("C:\\Program Files (x86)\\VB\\Voicemeeter", ec)
            || std::filesystem::exists("C:\\Program Files\\VB\\Voicemeeter", ec))
        {
            return true;
        }

        HKEY hKey = nullptr;
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}",
                            0, KEY_READ | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
        {
            ::RegCloseKey(hKey);
            return true;
        }

        return false;
    }
#endif
} // namespace

DependencyStatus SystemDependencyChecker::inspectSystem(const AudioPolicyManager& policyManager)
{
    DependencyStatus status {};
    std::error_code ec;

    std::filesystem::path apoRoot = "C:\\Program Files\\EqualizerAPO";
#ifdef _WIN32
    status.isRunningAsAdmin = checkProcessElevated();
    const auto regApoDir = queryRegistryApoInstallDir();
    if (!regApoDir.empty() && std::filesystem::exists(regApoDir, ec))
        apoRoot = regApoDir;
#endif

    // 1. Verificar EqualizerAPO.dll
    status.apoDllPath = apoRoot / "EqualizerAPO.dll";
    status.hasEqualizerApo = std::filesystem::exists(status.apoDllPath, ec);

    // 2. Verificar carpeta HeSuVi
    status.hesuviDirPath = apoRoot / "config" / "HeSuVi";
    status.hasHeSuVi = std::filesystem::exists(status.hesuviDirPath, ec)
                    && std::filesystem::is_directory(status.hesuviDirPath, ec);

    // 3. Verificar driver VB-Audio Hi-Fi Cable / VB-Cable / VoiceMeeter en endpoints activos WASAPI
    const auto endpoints = policyManager.enumerateActiveRenderEndpoints();
    int bestPriority = 99;

    for (const auto& ep : endpoints)
    {
        const std::wstring lower = toLowerWide(ep.friendlyName);

        int priority = 99;
        if (lower.find(L"hi-fi cable input") != std::wstring::npos)
            priority = 1;
        else if (lower.find(L"cable input") != std::wstring::npos)
            priority = 2;
        else if (lower.find(L"vb-audio") != std::wstring::npos || lower.find(L"voicemeeter") != std::wstring::npos)
            priority = 3;

        if (priority < bestPriority)
        {
            bestPriority = priority;
            status.hasVbAudioCable         = true;
            status.detectedCableName       = ep.friendlyName;
            status.detectedCableEndpointId = ep.id;
            status.detectedCableGuid       = ep.endpointGuid;
        }
    }

#ifdef _WIN32
    if (!status.hasVbAudioCable && checkVbDriverInRegistryOrDisk())
    {
        status.hasVbAudioCable   = true;
        status.detectedCableName = L"VB-Audio Driver (Instalado, verificar dispositivo habilitado en mmsys.cpl)";
    }
#endif

    return status;
}

void SystemDependencyChecker::openOfficialDownloadLinks()
{
#ifdef _WIN32
    ::ShellExecuteW(nullptr, L"open", kEqualizerApoUrl, nullptr, nullptr, SW_SHOWNORMAL);
    ::ShellExecuteW(nullptr, L"open", kVbCableUrl,      nullptr, nullptr, SW_SHOWNORMAL);
    ::ShellExecuteW(nullptr, L"open", kHeSuViUrl,       nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

bool SystemDependencyChecker::relaunchAsAdministrator()
{
#ifdef _WIN32
    wchar_t exePath[MAX_PATH] {};
    if (::GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0)
        return false;

    const HINSTANCE res = ::ShellExecuteW(nullptr, L"runas", exePath, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(res) > 32;
#else
    return false;
#endif
}

std::string SystemDependencyChecker::buildInstallationGuideText(const DependencyStatus& status)
{
    std::ostringstream out;

    out << "=== ESTADO ACTUAL DE DEPENDENCIAS ===\n"
        << "  [" << (status.hasEqualizerApo ? "OK" : "FALTA") << "] Equalizer APO (C:\\Program Files\\EqualizerAPO\\EqualizerAPO.dll)\n"
        << "  [" << (status.hasVbAudioCable ? "OK" : "FALTA") << "] Driver Virtual VB-Audio Hi-Fi Cable / VoiceMeeter\n"
        << "  [" << (status.hasHeSuVi       ? "OK" : "FALTA") << "] Motor Binaural HeSuVi (C:\\Program Files\\EqualizerAPO\\config\\HeSuVi)\n"
        << "  [" << (status.isRunningAsAdmin ? "ADMIN" : "ESTANDAR") << "] Privilegios de Escritura en Program Files\n\n"
        << "=== GUIA PASO A PASO DE INSTALACION TACTICA ===\n"
        << "PASO 1 - Instalar VB-Audio Hi-Fi Cable (o VB-Cable):\n"
        << "  1. Extrae el ZIP oficial de VB-Audio, haz clic derecho en Setup_x64.exe -> 'Ejecutar como Administrador'.\n"
        << "  2. Abre el Panel de Sonido clasico (Win+R -> mmsys.cpl), selecciona 'Hi-Fi Cable Input' -> Configurar -> '7.1 Surround'.\n"
        << "  3. En la pestana 'Grabar' -> 'Hi-Fi Cable Output' -> Propiedades -> Escuchar -> Marca 'Escuchar este dispositivo' hacia tus auriculares fisicos.\n\n"
        << "PASO 2 - Instalar Equalizer APO:\n"
        << "  1. Instala Equalizer APO en su ruta estandar C:\\Program Files\\EqualizerAPO.\n"
        << "  2. En el 'Configurator' que se abre al final, marca la casilla de 'Hi-Fi Cable Input' (y tus auriculares si deseas fallback directo).\n\n"
        << "PASO 3 - Instalar HeSuVi y Copiar los 3 Plugins VST:\n"
        << "  1. Ejecuta el instalador de HeSuVi (se extrae automaticamente en C:\\Program Files\\EqualizerAPO\\config\\HeSuVi).\n"
        << "  2. Copia TransientSuppressor.vst3 / .dll, LoudnessCompressor.vst3 / .dll y AmbienceGuard.vst3 / .dll dentro de:\n"
        << "     C:\\Program Files\\EqualizerAPO\\VSTPlugins\\\n\n"
        << "PASO 4 - Reinicio Obligatorio de Windows:\n"
        << "  1. Reinicia tu PC para que el subsistema WASAPI cargue EqualizerAPO.dll y el driver virtual.\n"
        << "  2. Abre 'Dash Competitivo Audio' como Administrador y pulsa 'Activar Perfil'.";

    return out.str();
}

} // namespace dash::audio
