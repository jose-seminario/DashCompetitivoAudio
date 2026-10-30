#include "ProfileRepository.h"
#include "ProfileSerializer.h"
#include "ProfileValidator.h"

#include <cstdlib>
#include <system_error>

namespace dash::profile
{

std::filesystem::path ProfileRepository::getDefaultUserProfilesDir()
{
#ifdef _WIN32
    char* localAppData = nullptr;
    size_t len = 0;
    if (_dupenv_s(&localAppData, &len, "LOCALAPPDATA") == 0 && localAppData != nullptr)
    {
        std::filesystem::path p = std::filesystem::path(localAppData) / "DashCompetitivoAudio" / "Profiles";
        free(localAppData);
        return p;
    }
#endif
    return std::filesystem::current_path() / "Profiles" / "User";
}

ProfileRepository::ProfileRepository()
    : ProfileRepository(getDefaultUserProfilesDir())
{
}

ProfileRepository::ProfileRepository(std::filesystem::path userProfilesDir)
    : userProfilesDir_(std::move(userProfilesDir))
{
    std::error_code ec;
    std::filesystem::create_directories(userProfilesDir_, ec);
    searchDirectories_.push_back(userProfilesDir_);
}

void ProfileRepository::addSearchDirectory(const std::filesystem::path& dir)
{
    if (!dir.empty() && std::filesystem::exists(dir))
    {
        searchDirectories_.push_back(dir);
    }
}

std::filesystem::path ProfileRepository::resolveProfilePath(const std::string& profileId) const
{
    if (!ProfileValidator::isSafeProfileId(profileId))
        return {};

    const std::string filename = profileId + ".json";

    // 1. Prioridad: directorio de usuario
    const auto userPath = userProfilesDir_ / filename;
    if (std::filesystem::exists(userPath))
        return userPath;

    // 2. Buscar en directorios registrados (recursivo seguro)
    for (const auto& dir : searchDirectories_)
    {
        if (dir == userProfilesDir_)
            continue;

        const auto directPath = dir / filename;
        if (std::filesystem::exists(directPath))
            return directPath;

        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec))
        {
            if (entry.is_regular_file() && entry.path().filename() == filename)
                return entry.path();
        }
    }

    return {};
}

std::vector<ProfileSummary> ProfileRepository::scanProfiles()
{
    std::vector<ProfileSummary> summaries;

    for (const auto& dir : searchDirectories_)
    {
        if (!std::filesystem::exists(dir))
            continue;

        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".json")
                continue;

            AudioProfile profile;
            std::string err;
            if (ProfileSerializer::loadFromFile(entry.path(), profile, &err))
            {
                ProfileSummary summary;
                summary.profileId      = profile.profileId;
                summary.name           = profile.name;
                summary.gameId         = profile.game.id;
                summary.headphoneModel = profile.headphone.manufacturer + " " + profile.headphone.model;
                summary.verified       = profile.metadata.verified;
                summary.filePath       = entry.path();
                summary.isCustomUser   = (entry.path().parent_path() == userProfilesDir_);

                summaries.push_back(std::move(summary));
            }
        }
    }

    return summaries;
}

bool ProfileRepository::loadProfileById(const std::string& profileId, AudioProfile& outProfile, std::string* outError)
{
    if (!ProfileValidator::isSafeProfileId(profileId))
    {
        if (outError)
            *outError = "Identificador de perfil inseguro o no válido: '" + profileId + "'";
        return false;
    }

    const auto resolved = resolveProfilePath(profileId);
    if (resolved.empty())
    {
        if (outError)
            *outError = "Perfil no encontrado en los repositorios locales: " + profileId;
        return false;
    }

    return ProfileSerializer::loadFromFile(resolved, outProfile, outError);
}

bool ProfileRepository::saveUserProfile(const AudioProfile& profile, std::string* outError)
{
    if (!ProfileValidator::isSafeProfileId(profile.profileId))
    {
        if (outError)
            *outError = "El ID del perfil no es válido para guardado en disco: '" + profile.profileId + "'";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(userProfilesDir_, ec);

    const auto targetPath = userProfilesDir_ / (profile.profileId + ".json");
    return ProfileSerializer::saveToFile(profile, targetPath, outError);
}

bool ProfileRepository::deleteUserProfile(const std::string& profileId, std::string* outError)
{
    if (!ProfileValidator::isSafeProfileId(profileId))
    {
        if (outError)
            *outError = "Identificador no seguro para eliminación.";
        return false;
    }

    const auto userPath = userProfilesDir_ / (profileId + ".json");
    if (!std::filesystem::exists(userPath))
    {
        if (outError)
            *outError = "No se puede eliminar: el archivo no existe en la carpeta de usuario.";
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::remove(userPath, ec))
    {
        if (outError)
            *outError = "Error al eliminar archivo: " + ec.message();
        return false;
    }

    return true;
}

bool ProfileRepository::importProfile(const std::filesystem::path& sourceFilePath, AudioProfile& outProfile, std::string* outError)
{
    std::error_code ec;
    if (std::filesystem::is_symlink(sourceFilePath, ec))
    {
        if (outError)
            *outError = "Rechazado por seguridad: el archivo de perfil es un enlace simbólico no permitido.";
        return false;
    }

    if (!ProfileSerializer::loadFromFile(sourceFilePath, outProfile, outError))
        return false;

    // Guardar copia local en el almacén de perfiles de usuario
    return saveUserProfile(outProfile, outError);
}

bool ProfileRepository::exportProfile(const std::string& profileId, const std::filesystem::path& destinationPath, std::string* outError)
{
    AudioProfile profile;
    if (!loadProfileById(profileId, profile, outError))
        return false;

    return ProfileSerializer::saveToFile(profile, destinationPath, outError);
}

} // namespace dash::profile
