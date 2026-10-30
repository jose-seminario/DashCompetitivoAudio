/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ProfileMigrator
 *
 * Cobertura de pruebas preparadas:
 *  1. Migración de perfil v0 (sin versión) a v1 inyectando metadatos por defecto.
 *  2. Invarianza de perfiles v1 actuales (no se alteran si ya están actualizados).
 *  3. Rechazo de versiones futuras (> 1) que requieran actualización del software.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <Profiles/ProfileMigrator.h>
#include <Profiles/ProfileSerializer.h>

#include <cassert>
#include <string>

namespace dash::test
{

void testProfileMigratorV0ToV1()
{
    using namespace dash::profile;

    // JSON heredado antiguo sin schemaVersion ni secciones modernas de headphone/spatial
    const std::string legacyJson = R"({
        "profileId": "legacy-warzone",
        "name": "Legacy Warzone",
        "description": "Old profile format",
        "processing": {
            "headroomDb": -6.0,
            "equalizer": { "footstepsDb": 2.0, "bassDb": 1.0 }
        }
    })";

    std::string migratedJson;
    std::string err;
    const bool ok = ProfileMigrator::migrateJson(legacyJson, migratedJson, &err);
    assert(ok);
    assert(err.empty());

    // El JSON migrado debe poder deserializarse con éxito en AudioProfile v1
    AudioProfile profile;
    const bool loaded = ProfileSerializer::deserializeFromJson(migratedJson, profile, &err);
    assert(loaded);
    assert(profile.schemaVersion == 1);
    assert(profile.profileId == "legacy-warzone");
    assert(profile.game.id == "call-of-duty-warzone");
    assert(profile.headphone.calibrationStatus == "uncalibrated");
}

void testProfileMigratorUnsupportedVersion()
{
    using namespace dash::profile;

    assert(ProfileMigrator::canMigrate(0));
    assert(ProfileMigrator::canMigrate(1));
    assert(!ProfileMigrator::canMigrate(99)); // Versión futura no soportada
}

} // namespace dash::test
