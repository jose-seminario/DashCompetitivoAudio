#pragma once

#include <filesystem>
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
    static constexpr const wchar_t* kEqualizerApoUrl        = L"https://sourceforge.net/projects/equalizerapo/";
    static constexpr const wchar_t* kVbCableUrl             = L"https://vb-audio.com/Cable/";
    static constexpr const wchar_t* kHeSuViUrl              = L"https://sourceforge.net/projects/hesuvi/";
    static constexpr const wchar_t* kHeSuViDirectDownloadUrl =
        L"https://sourceforge.net/projects/hesuvi/files/HeSuVi_2.0.0.1.exe/download";
    static constexpr const wchar_t* kHeSuViInstallerFilename = L"HeSuVi_2.0.0.1.exe";

    /**
     * Verifica en disco (std::filesystem), Registro de Windows y endpoints WASAPI
     * la presencia de Equalizer APO, VB-Audio Hi-Fi Cable / VoiceMeeter y HeSuVi.
     */
    static DependencyStatus inspectSystem(const AudioPolicyManager& policyManager);

    /**
     * Abre en el navegador los enlaces de las dependencias faltantes (Equalizer APO / VB-Cable)
     * mediante ShellExecuteW.
     */
    static void openOfficialDownloadLinks(bool openApo = true, bool openVbCable = true, bool openHeSuViPage = false);

    /**
     * Descarga automáticamente HeSuVi_2.0.0.1.exe desde SourceForge usando URLDownloadToFileW
     * en %TEMP%\HeSuVi_2.0.0.1.exe, verifica que sea un ejecutable PE válido y lo lanza;
     * si un firewall bloquea la descarga directa, hace fallback transparente con ShellExecuteW.
     */
    static bool downloadAndLaunchHeSuViInstaller(
        std::filesystem::path* outInstallerPath = nullptr,
        std::string* outMessage = nullptr);

    /**
     * Intenta relanzar el ejecutable actual solicitando elevación UAC ("runas").
     */
    static bool relaunchAsAdministrator();

    /**
     * Devuelve el tutorial paso a paso de instalación y configuración 7.1.
     */
    static std::string buildInstallationGuideText(const DependencyStatus& status);
};

} // namespace dash::audio
