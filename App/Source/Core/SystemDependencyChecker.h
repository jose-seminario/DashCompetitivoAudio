#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "AudioPolicyManager.h"

namespace dash::audio
{

struct DependencyStatus
{
    bool hasEqualizerApo  = false;
    bool hasVbAudioCable  = false;
    bool hasHeSuVi        = false;
    bool isRunningAsAdmin = false;

    std::filesystem::path apoDllPath;
    std::filesystem::path hesuviDirPath;
    std::wstring detectedCableName;
    std::wstring detectedCableEndpointId;
    std::wstring detectedCableGuid;

    [[nodiscard]] bool allDependenciesReady() const noexcept
    {
        return hasEqualizerApo && hasVbAudioCable && hasHeSuVi;
    }
};

class SystemDependencyChecker
{
public:
    static constexpr const wchar_t* kEqualizerApoUrl = L"https://sourceforge.net/projects/equalizerapo/";
    static constexpr const wchar_t* kVbCableUrl      = L"https://vb-audio.com/Cable/";
    static constexpr const wchar_t* kHeSuViUrl       = L"https://sourceforge.net/projects/hesuvi/";

    static constexpr const wchar_t* kEqualizerApoInstallerUrl = L"https://sourceforge.net/projects/equalizerapo/files/1.3.2/EqualizerAPO64-1.3.2.exe/download";
    static constexpr const wchar_t* kVbCableZipUrl            = L"https://download.vb-audio.com/Download_CABLENew/VBCABLE_Driver_Pack43.zip";

    /**
     * Verifica en disco (std::filesystem), Registro de Windows y endpoints WASAPI
     * la presencia de Equalizer APO, VB-Audio Hi-Fi Cable / VoiceMeeter y HeSuVi.
     */
    static DependencyStatus inspectSystem(const AudioPolicyManager& policyManager);

    /**
     * Abre en el navegador los enlaces oficiales para que el usuario descargue e instale manualmente.
     */
    static void openOfficialDownloadLinks(bool openApo = true, bool openVbCable = true, bool openHeSuViPage = true);

    /**
     * Descarga e instala automáticamente las dependencias ausentes utilizando URLDownloadToFileW
     * hacia %TEMP% y ejecutando los instaladores con elevación UAC ("runas") mediante ShellExecuteW.
     * Al finalizar, retorna un mensaje solicitando el reinicio del equipo.
     */
    static bool downloadAndInstallDependencies(
        const DependencyStatus& status,
        std::string* outMessage = nullptr,
        std::function<void(const std::string& msg, bool isError)> progressCallback = nullptr);

    /**
     * Intenta relanzar el ejecutable actual solicitando elevación UAC ("runas").
     */
    static bool relaunchAsAdministrator();

    /**
     * Devuelve el tutorial paso a paso de instalación y configuración manual.
     */
    static std::string buildInstallationGuideText(const DependencyStatus& status);
};

} // namespace dash::audio
