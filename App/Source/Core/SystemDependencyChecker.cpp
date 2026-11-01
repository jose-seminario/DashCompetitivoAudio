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

    // 1. Verificar presencia de EqualizerAPO.dll
    status.apoDllPath = apoRoot / "EqualizerAPO.dll";
    status.hasEqualizerApo = std::filesystem::exists(status.apoDllPath, ec);

    // 2. Verificar carpeta HeSuVi y archivo hesuvi.txt
    status.hesuviDirPath = apoRoot / "config" / "HeSuVi";
    status.hasHeSuVi = std::filesystem::exists(status.hesuviDirPath / "hesuvi.txt", ec)
                    || (std::filesystem::exists(status.hesuviDirPath, ec)
                        && std::filesystem::is_directory(status.hesuviDirPath, ec));

    // 3. Verificar driver VB-Audio Hi-Fi Cable / VB-Cable / VoiceMeeter en endpoints WASAPI activos
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
        ::ShellExecuteW(nullptr, L"open", kHeSuViUrl, nullptr, nullptr, SW_SHNORMAL);
#else
    (void)openApo;
    (void)openVbCable;
    (void)openHeSuViPage;
#endif
}

bool SystemDependencyChecker::downloadAndInstallDependencies(
    const DependencyStatus& status,
    std::string* outMessage,
    std::function<void(const std::string& msg, bool isError)> progressCallback)
{
#ifdef _WIN32
    std::error_code ec;
    const auto tempDir = std::filesystem::temp_directory_path(ec);
    if (ec)
    {
        if (outMessage) *outMessage = "Error al acceder al directorio temporal %TEMP%.";
        if (progressCallback) progressCallback("Error al acceder a %TEMP%", true);
        return false;
    }

    bool anyInstalled = false;

    // 1. Descarga e instalación de Equalizer APO
    if (!status.hasEqualizerApo)
    {
        if (progressCallback)
            progressCallback("Descargando instalador oficial de Equalizer APO a %TEMP%...", false);

        const auto apoPath = tempDir / "EqualizerAPO64-1.3.2.exe";
        const HRESULT hr = ::URLDownloadToFileW(nullptr, kEqualizerApoInstallerUrl, apoPath.c_str(), 0, nullptr);
        if (SUCCEEDED(hr))
        {
            if (progressCallback)
                progressCallback("Ejecutando instalador de Equalizer APO (solicitando elevación UAC)...", false);

            const HINSTANCE res = ::ShellExecuteW(nullptr, L"runas", apoPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            if (reinterpret_cast<intptr_t>(res) > 32)
            {
                anyInstalled = true;
            }
            else
            {
                if (progressCallback)
                    progressCallback("Instalador de Equalizer APO cancelado o denegado por UAC.", true);
            }
        }
        else
        {
            if (progressCallback)
                progressCallback("Fallo al descargar Equalizer APO. Abriendo enlace oficial...", true);
            openOfficialDownloadLinks(true, false, false);
        }
    }

    // 2. Descarga e instalación de VB-Audio Cable
    if (!status.hasVbAudioCable)
    {
        if (progressCallback)
            progressCallback("Descargando paquete oficial de drivers VB-Audio Cable...", false);

        const auto zipPath = tempDir / "VBCABLE_Driver_Pack43.zip";
        const auto extractDir = tempDir / "VBCABLE_Driver_Pack43";

        const HRESULT hr = ::URLDownloadToFileW(nullptr, kVbCableZipUrl, zipPath.c_str(), 0, nullptr);
        if (SUCCEEDED(hr))
        {
            if (progressCallback)
                progressCallback("Extrayendo paquete de VB-Audio Cable...", false);

            std::wstring psCmd = L"-NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -Path '" + zipPath.wstring() + L"' -DestinationPath '" + extractDir.wstring() + L"' -Force\"";

            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.lpVerb = L"open";
            sei.lpFile = L"powershell.exe";
            sei.lpParameters = psCmd.c_str();
            sei.nShow = SW_HIDE;
            if (::ShellExecuteExW(&sei) && sei.hProcess)
            {
                ::WaitForSingleObject(sei.hProcess, 15000);
                ::CloseHandle(sei.hProcess);
            }

            auto setupExe = extractDir / "VBCABLE_Setup_x64.exe";
            if (!std::filesystem::exists(setupExe, ec))
                setupExe = extractDir / "VBCABLE_Driver_Pack43" / "VBCABLE_Setup_x64.exe";

            if (std::filesystem::exists(setupExe, ec))
            {
                if (progressCallback)
                    progressCallback("Ejecutando instalador de VB-Audio Cable (solicitando elevación UAC)...", false);

                const HINSTANCE res = ::ShellExecuteW(nullptr, L"runas", setupExe.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                if (reinterpret_cast<intptr_t>(res) > 32)
                {
                    anyInstalled = true;
                }
                else
                {
                    if (progressCallback)
                        progressCallback("Instalador de VB-Cable cancelado o denegado por UAC.", true);
                }
            }
            else
            {
                openOfficialDownloadLinks(false, true, false);
            }
        }
        else
        {
            if (progressCallback)
                progressCallback("Fallo al descargar VB-Cable. Abriendo enlace oficial...", true);
            openOfficialDownloadLinks(false, true, false);
        }
    }

    const std::string finalMsg = "Instalación completada. Por favor, REINICIA tu computadora para activar los drivers virtuales";
    if (outMessage)
        *outMessage = finalMsg;
    if (progressCallback)
        progressCallback(finalMsg, false);

    return anyInstalled;
#else
    (void)status;
    if (outMessage) *outMessage = "Instalación automática solo disponible en Windows.";
    if (progressCallback) progressCallback("Instalación automática solo disponible en Windows.", true);
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
        << "  [" << (status.hasHeSuVi       ? "OK" : "FALTA") << "] Motor Binaural HeSuVi (Include: HeSuVi\\hesuvi.txt)\n"
        << "  [" << (status.isRunningAsAdmin ? "ADMIN" : "ESTANDAR") << "] Privilegios de Escritura en Program Files\n\n"
        << "=== GUIA PASO A PASO DE INSTALACION MANUAL ===\n"
        << "PASO 1 - Instalar VB-Audio Hi-Fi Cable (o VB-Cable):\n"
        << "  1. Descarga el paquete desde https://vb-audio.com/Cable/ y descomprímelo.\n"
        << "  2. Haz clic derecho en Setup_x64.exe -> 'Ejecutar como Administrador'.\n"
        << "  3. Abre mmsys.cpl (Win+R -> mmsys.cpl), selecciona 'Hi-Fi Cable Input' -> Configurar -> '7.1 Surround'.\n"
        << "  4. En la pestaña 'Grabar' -> 'Hi-Fi Cable Output' -> Propiedades -> Escuchar -> Marca 'Escuchar este dispositivo' hacia tus auriculares.\n\n"
        << "PASO 2 - Instalar Equalizer APO:\n"
        << "  1. Descarga desde https://sourceforge.net/projects/equalizerapo/ e instala en C:\\Program Files\\EqualizerAPO.\n"
        << "  2. En el 'Configurator', marca la casilla de 'Hi-Fi Cable Input'.\n\n"
        << "PASO 3 - Instalar HeSuVi (Opcional para Virtualización 7.1):\n"
        << "  1. Descarga HeSuVi desde https://sourceforge.net/projects/hesuvi/.\n"
        << "  2. Ejecuta el instalador para extraer los archivos en C:\\Program Files\\EqualizerAPO\\config\\HeSuVi\\.\n\n"
        << "PASO 4 - Reinicio de Windows y Activación:\n"
        << "  1. Reinicia tu PC para registrar los controladores en el subsistema WASAPI.\n"
        << "  2. Inicia 'Dash Competitivo Audio', selecciona tu perfil y haz clic en 'Activar Perfil'.";

    return out.str();
}

} // namespace dash::audio
