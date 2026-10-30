/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para AudioProfile
 *
 * Cobertura de pruebas preparadas:
 *  1. Inicialización nominal por defecto: valores de fábrica seguros.
 *  2. Validación de estado interno: rechazo de perfiles sin ID o sin nombre.
 *  3. Integridad de parámetros de ecualizador gráfico y paramétrico.
 *  4. Compatibilidad con versión actual de esquema (schemaVersion = 1).
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <Profiles/AudioProfile.h>

#include <cassert>
#include <string>

namespace dash::test
{

void testAudioProfileDefaults()
{
    using namespace dash::profile;

    AudioProfile profile;
    profile.resetToDefaults();

    assert(profile.schemaVersion == AudioProfile::kCurrentSchemaVersion);
    assert(profile.profileId == "default-profile");
    assert(!profile.name.empty());
    assert(profile.isValid());

    // Headroom nominal
    assert(profile.dspParams.headroomDb == -6.0f);
    assert(profile.dspParams.limiter.ceilingDb == -0.5f);

    // Valores tácticos por defecto
    assert(profile.dspParams.equalizer.footstepsDb == 1.6f);
    assert(profile.dspParams.equalizer.bassDb == 1.5f);
    assert(profile.dspParams.transient.centerReductionDb == 4.0f);
    assert(profile.dspParams.ambience.reductionDb == 6.0f);
    assert(profile.dspParams.dynamics.targetLufs == -16.0f);
}

void testAudioProfileValidation()
{
    using namespace dash::profile;

    AudioProfile profile;
    profile.resetToDefaults();
    assert(profile.isValid());

    // ID vacío debe invalidar el perfil
    profile.profileId = "";
    assert(!profile.isValid());

    profile.profileId = "valid-slug-id";
    assert(profile.isValid());

    // Nombre vacío debe invalidar
    profile.name = "";
    assert(!profile.isValid());

    profile.name = "Warzone Custom";
    assert(profile.isValid());

    // Versión de esquema futura debe invalidar
    profile.schemaVersion = 999;
    assert(!profile.isValid());
}

} // namespace dash::test
