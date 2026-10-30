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
    static constexpr const wchar_t* kEqualizerApoUrl = L"https://sourceforge.net/projects/equalizerapo/";
    static constexpr const wchar_t* kVbCableUrl      = L"https://vb-audio.com/Cable/";
    static constexpr const wchar_t* kHeSuViUrl       = L"https://sourceforge.net/projects/hesuvi/";

    /**
     * Verifica en disco (std::filesystem), Registro de Windows y endpoints WASAPI
     * la presencia de Equalizer APO, VB-Audio Hi-Fi Cable / VoiceMeeter y HeSuVi.
     */
    static DependencyStatus inspectSystem(const AudioPolicyManager& policyManager);

    /**
     * Abre en el navegador los enlaces oficiales para que el usuario descargue e instale manualmente.
     * Nunca descarga ejecutables ni instala software automáticamente.
     */
    static void openOfficialDownloadLinks(bool openApo = true, bool openVbCable = true, bool openHeSuViPage = true);

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
