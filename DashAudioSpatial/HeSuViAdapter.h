#pragma once

#include "ChannelLayout.h"
#include "HRIRDataset.h"

#include <filesystem>
#include <string>
#include <vector>

namespace dash::spatial
{

struct HeSuViProfileInfo
{
    std::string name;
    std::filesystem::path filePath;
    size_t numChannels = 0;
    size_t lengthTaps  = 0;
    double sampleRate  = 0.0;
    bool isValid       = false;
};

/**
 * Adaptador de Integración y Compatibilidad con HeSuVi (HeSuViAdapter).
 *
 * Principios de seguridad y propiedad intelectual:
 *  - NO copia, no empaqueta ni redistribuye perfiles ni archivos de audio propietarios de HeSuVi.
 *  - Detecta la instalación preexistente del usuario en el sistema operativo.
 *  - Permite importar un archivo HRIR WAV de HeSuVi hacia la memoria de HRIRDataset
 *    estrictamente fuera del hilo de audio.
 *  - Mapea el layout de 14 canales estándar de HeSuVi hacia los altavoces de ChannelLayout::surround7_1().
 */
class HeSuViAdapter
{
public:
    static constexpr const char* kDefaultRelativeHeSuViDir = "HeSuVi";
    static constexpr const char* kDefaultHrirSubDir         = "HeSuVi\\hrir";

    /**
     * Localiza el directorio de instalación de HeSuVi en el sistema.
     */
    [[nodiscard]] static std::filesystem::path detectHeSuViPath();

    /**
     * Enumera los perfiles HRIR .wav disponibles en la carpeta HeSuVi\hrir del usuario.
     */
    [[nodiscard]] static std::vector<HeSuViProfileInfo> enumerateInstalledProfiles(
        const std::filesystem::path& customHeSuViPath = {});

    /**
     * Comprueba si HeSuVi está instalado y disponible.
     */
    [[nodiscard]] static bool isHeSuViInstalled();

    /**
     * Carga un archivo WAV multicanal de HeSuVi y lo transfiere a un HRIRDataset en memoria.
     */
    static bool loadProfileIntoDataset(
        const std::filesystem::path& wavFilePath,
        HRIRDataset& outDataset,
        std::string* outError = nullptr);
};

} // namespace dash::spatial
