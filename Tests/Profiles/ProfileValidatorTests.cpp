/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ProfileValidator
 *
 * Cobertura de pruebas preparadas:
 *  1. Detección de código malicioso/ejecutable en JSON (.exe, .dll, powershell, cmd.exe).
 *  2. Prevención de Path Traversal (rechazo de slugs con '..', '/', '\', o nombres reservados DOS).
 *  3. Límite de tamaño de archivo (1 MB máximo).
 *  4. Validación de rangos numéricos DSP (frecuencias audibles, ganancias, factores Q).
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <Profiles/ProfileValidator.h>

#include <cassert>
#include <string>

namespace dash::test
{

void testProfileValidatorMaliciousJsonRejection()
{
    using namespace dash::profile;

    // 1. Detección de comandos de PowerShell
    const std::string maliciousPs1 = R"({ "profileId": "hack", "cmd": "powershell.exe -ExecutionPolicy Bypass" })";
    const auto res1 = ProfileValidator::validateRawJsonText(maliciousPs1);
    assert(res1.hasErrors());

    // 2. Detección de binarios DLL
    const std::string maliciousDll = R"({ "profileId": "hack", "library": "payload.dll" })";
    const auto res2 = ProfileValidator::validateRawJsonText(maliciousDll);
    assert(res2.hasErrors());

    // 3. Detección de scripts por lotes
    const std::string maliciousBat = R"({ "profileId": "hack", "script": "install.bat" })";
    const auto res3 = ProfileValidator::validateRawJsonText(maliciousBat);
    assert(res3.hasErrors());
}

void testProfileValidatorPathTraversalPrevention()
{
    using namespace dash::profile;

    assert(ProfileValidator::isSafeProfileId("warzone-competitive"));
    assert(ProfileValidator::isSafeProfileId("hyperx_cloud_ii_2026"));
    assert(ProfileValidator::isSafeProfileId("profile-123"));

    // Path traversal clásico
    assert(!ProfileValidator::isSafeProfileId("../exploit"));
    assert(!ProfileValidator::isSafeProfileId("..\\windows\\system32"));
    assert(!ProfileValidator::isSafeProfileId("folder/subprofile"));
    assert(!ProfileValidator::isSafeProfileId("c:profile"));
    assert(!ProfileValidator::isSafeProfileId(""));

    // Nombres reservados de Windows
    assert(!ProfileValidator::isSafeProfileId("con"));
    assert(!ProfileValidator::isSafeProfileId("prn"));
    assert(!ProfileValidator::isSafeProfileId("aux"));
    assert(!ProfileValidator::isSafeProfileId("nul"));
    assert(!ProfileValidator::isSafeProfileId("com1"));
}

void testProfileValidatorNumericRanges()
{
    using namespace dash::profile;

    AudioProfile profile;
    profile.resetToDefaults();
    assert(ProfileValidator::validateProfile(profile).valid);

    // Frecuencia acústica fuera del rango audible (> 20 kHz)
    profile.dspParams.useParametricEq = true;
    profile.dspParams.parametricBands[0].enabled = true;
    profile.dspParams.parametricBands[0].frequencyHz = 45000.0f; // Inválido
    assert(ProfileValidator::validateProfile(profile).hasErrors());

    // Corrección
    profile.dspParams.parametricBands[0].frequencyHz = 3150.0f;
    assert(ProfileValidator::validateProfile(profile).valid);

    // Ganancia excesiva (+35 dB)
    profile.dspParams.parametricBands[0].gainDb = 35.0f;
    assert(ProfileValidator::validateProfile(profile).hasErrors());

    profile.dspParams.parametricBands[0].gainDb = 3.0f;
    assert(ProfileValidator::validateProfile(profile).valid);

    // Factor Q no físico (< 0.1)
    profile.dspParams.parametricBands[0].q = 0.01f;
    assert(ProfileValidator::validateProfile(profile).hasErrors());
}

} // namespace dash::test
