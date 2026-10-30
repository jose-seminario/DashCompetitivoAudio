#pragma once

#include "AudioProfile.h"

#include <string>

namespace dash::profile
{

/**
 * Migrador de Versiones de Esquema de Perfiles (ProfileMigrator).
 *
 * Permite la evolución del formato JSON sin romper compatibilidad con perfiles
 * creados por usuarios o versiones previas de la plataforma.
 */
class ProfileMigrator
{
public:
    /**
     * Determina si una versión de esquema puede ser migrada a la versión actual.
     */
    [[nodiscard]] static bool canMigrate(int sourceVersion) noexcept;

    /**
     * Migra una representación JSON cruda de versión previa a la versión canónica actual.
     */
    static bool migrateJson(const std::string& sourceJson, std::string& outMigratedJson, std::string* outError = nullptr);

    /**
     * Migra un objeto AudioProfile en memoria a la versión actual.
     */
    static bool migrateProfile(AudioProfile& profile, std::string* outError = nullptr);
};

} // namespace dash::profile
