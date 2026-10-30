#pragma once

#include "AudioProfile.h"

#include <filesystem>
#include <string>
#include <vector>

namespace dash::profile
{

/**
 * Resumen ligero de un perfil para presentación en la interfaz de usuario (ComboBox / Lista).
 */
struct ProfileSummary
{
    std::string profileId;
    std::string name;
    std::string gameId;
    std::string headphoneModel;
    bool        isCustomUser = false;
    bool        verified     = false;
    std::filesystem::path filePath;
};

/**
 * Administrador y Repositorio de Perfiles Locales (ProfileRepository).
 *
 * Desacopla la persistencia en disco de los componentes gráficos y el motor de audio.
 * Garantiza acceso seguro evitando path traversal y aislando perfiles de sistema.
 */
class ProfileRepository
{
public:
    ProfileRepository();
    explicit ProfileRepository(std::filesystem::path userProfilesDir);

    /**
     * Añade un directorio adicional de búsqueda de perfiles (ej: perfiles de fábrica o biblioteca comunitaria).
     */
    void addSearchDirectory(const std::filesystem::path& dir);

    /**
     * Escanea todos los directorios registrados y retorna los resúmenes de perfiles encontrados.
     */
    [[nodiscard]] std::vector<ProfileSummary> scanProfiles();

    /**
     * Carga un perfil por su identificador único (slug).
     */
    bool loadProfileById(const std::string& profileId, AudioProfile& outProfile, std::string* outError = nullptr);

    /**
     * Guarda o actualiza un perfil en el directorio de usuario.
     */
    bool saveUserProfile(const AudioProfile& profile, std::string* outError = nullptr);

    /**
     * Elimina un perfil personalizado de usuario (no permite borrar perfiles de fábrica de solo lectura).
     */
    bool deleteUserProfile(const std::string& profileId, std::string* outError = nullptr);

    /**
     * Importa un archivo de perfil externo, validándolo antes de copiarlo al almacén de usuario.
     */
    bool importProfile(const std::filesystem::path& sourceFilePath, AudioProfile& outProfile, std::string* outError = nullptr);

    /**
     * Exporta un perfil local a una ubicación arbitraria seleccionada por el usuario.
     */
    bool exportProfile(const std::string& profileId, const std::filesystem::path& destinationPath, std::string* outError = nullptr);

    /**
     * Retorna la ruta por defecto en %LOCALAPPDATA%/DashCompetitivoAudio/Profiles.
     */
    [[nodiscard]] static std::filesystem::path getDefaultUserProfilesDir();

    [[nodiscard]] const std::filesystem::path& getUserProfilesDirectory() const noexcept { return userProfilesDir_; }

private:
    std::filesystem::path userProfilesDir_;
    std::vector<std::filesystem::path> searchDirectories_;

    [[nodiscard]] std::filesystem::path resolveProfilePath(const std::string& profileId) const;
};

} // namespace dash::profile
