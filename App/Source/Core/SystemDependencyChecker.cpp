#include "SystemDependencyChecker.h"

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <urlmon.h>
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

    bool isValidWindowsExecutable(const std::filesystem::path& filePath)
    {
        std::error_code ec;
        const auto fileSize = std::filesystem::file_size(filePath, ec);
        if (ec || fileSize < 512 * 1024) // HeSuVi_2.0.0.1.exe (~34 MB SFX) debe superar ampliamente 512 KB
            return false;

        std::ifstream ifs(filePath, std::ios::binary);
        if (!ifs)
            return false;

        char mz[2] {};
        ifs.read(mz, 2);
        return ifs.gcount() == 2 && mz[0] == 'M' && mz[1] == 'Z';
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

    // 2. Verificar carpeta HeSuVi y archivo hesuvi.txt
    status.hesuviDirPath = apoRoot / "config" / "HeSuVi";
    status.hasHeSuVi = std::filesystem::exists(status.hesuviDirPath / "hesuvi.txt", ec)
                    || (std::filesystem::exists(status.hesuviDirPath, ec)
                        && std::filesystem::is_directory(status.hesuviDirPath, ec));

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

void SystemDependencyChecker::openOfficialDownloadLinks(bool openApo, bool openVbCable, bool openHeSuViPage)
{
#ifdef _WIN32
    if (openApo)
        ::ShellExecuteW(nullptr, L"open", kEqualizerApoUrl, nullptr, nullptr, SW_SHOWNORMAL);
    if (openVbCable)
        ::ShellExecuteW(nullptr, L"open", kVbCableUrl, nullptr, nullptr, SW_SHOWNORMAL);
    if (openHeSuViPage)
        ::ShellExecuteW(nullptr, L"open", kHeSuViUrl, nullptr, nullptr, SW_SHOWNORMAL);
#else
    (void)openApo;
    (void)openVbCable;
    (void)openHeSuViPage;
#endif
}

bool SystemDependencyChecker::downloadAndLaunchHeSuViInstaller(
    std::filesystem::path* outInstallerPath,
    std::string* outMessage)
{
#ifdef _WIN32
    std::error_code ec;
    std::filesystem::path tempDir = std::filesystem::temp_directory_path(ec);
    if (tempDir.empty())
        tempDir = "C:\\Windows\\Temp";

    const std::filesystem::path targetExe = tempDir / kHeSuViInstallerFilename;
    if (outInstallerPath)
        *outInstallerPath = targetExe;

    // Si ya se descargó previamente una copia íntegra de HeSuVi_2.0.0.1.exe, reutilizarla directamente
    if (!isValidWindowsExecutable(targetExe))
    {
        std::filesystem::remove(targetExe, ec);

        const HRESULT hr = ::URLDownloadToFileW(
            nullptr,
            kHeSuViDirectDownloadUrl,
            targetExe.wstring().c_str(),
            0,
            nullptr);

        if (FAILED(hr) || !isValidWindowsExecutable(targetExe))
        {
            std::filesystem::remove(targetExe, ec);

            // Fallback directo vía navegador hacia el mirror oficial de HeSuVi_2.0.0.1.exe
            ::ShellExecuteW(nullptr, L"open", kHeSuViDirectDownloadUrl, nullptr, nullptr, SW_SHOWNORMAL);

            if (outMessage)
                *outMessage = "Descarga de HeSuVi_2.0.0.1.exe iniciada en el navegador predeterminado (SourceForge).";
            return true;
        }
    }

    const HINSTANCE launchRes = ::ShellExecuteW(
        nullptr,
        L"open",
        targetExe.wstring().c_str(),
        nullptr,
        tempDir.wstring().c_str(),
        SW_SHOWNORMAL);

    if (reinterpret_cast<intptr_t>(launchRes) <= 32)
    {
        if (outMessage)
            *outMessage = "HeSuVi_2.0.0.1.exe descargado en '" + targetExe.string() + "', ejecútalo manualmente.";
        return false;
    }

    if (outMessage)
        *outMessage = "HeSuVi_2.0.0.1.exe descargado automáticamente y ejecutado desde '" + targetExe.string() + "'.";
    return true;
#else
    if (outInstallerPath) *outInstallerPath = {};
    if (outMessage) *outMessage = "Descarga automática disponible únicamente en Windows.";
    return false;
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
        << "  [" << (status.hasHeSuVi       ? "OK" : "FALTA -> Auto-descarga HeSuVi_2.0.0.1.exe lista") << "] Motor Binaural HeSuVi (Include: HeSuVi\\hesuvi.txt)\n"
        << "  [" << (status.isRunningAsAdmin ? "ADMIN" : "ESTANDAR") << "] Privilegios de Escritura en Program Files\n\n"
        << "=== GUIA PASO A PASO DE INSTALACION TACTICA ===\n"
        << "PASO 1 - Instalar VB-Audio Hi-Fi Cable (o VB-Cable):\n"
        << "  1. Extrae el ZIP oficial de VB-Audio, haz clic derecho en Setup_x64.exe -> 'Ejecutar como Administrador'.\n"
        << "  2. Abre el Panel de Sonido clasico (Win+R -> mmsys.cpl), selecciona 'Hi-Fi Cable Input' -> Configurar -> '7.1 Surround'.\n"
        << "  3. En la pestana 'Grabar' -> 'Hi-Fi Cable Output' -> Propiedades -> Escuchar -> Marca 'Escuchar este dispositivo' hacia tus auriculares fisicos.\n\n"
        << "PASO 2 - Instalar Equalizer APO:\n"
        << "  1. Instala Equalizer APO en C:\\Program Files\\EqualizerAPO.\n"
        << "  2. En el 'Configurator', marca la casilla de 'Hi-Fi Cable Input'.\n\n"
        << "PASO 3 - Instalar HeSuVi 2.0.0.1 (Descarga Automatica Integrada):\n"
        << "  1. Si falta HeSuVi, al pulsar 'Instalar Dependencias' la app descarga y ejecuta automaticamente HeSuVi_2.0.0.1.exe.\n"
        << "  2. El instalador extrae HeSuVi directamente en C:\\Program Files\\EqualizerAPO\\config\\HeSuVi\\hesuvi.txt.\n\n"
        << "PASO 4 - Reinicio Obligatorio de Windows:\n"
        << "  1. Reinicia tu PC para activar EqualizerAPO.dll y el driver virtual en el subsistema WASAPI.\n"
        << "  2. Abre 'Dash Competitivo Audio' y pulsa 'Activar Perfil'.";

    return out.str();
}

} // namespace dash::audio
