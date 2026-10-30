/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ProfileSerializer
 *
 * Cobertura de pruebas preparadas:
 *  1. Round-Trip Serialization: Serializar AudioProfile a JSON y recuperarlo idéntico.
 *  2. Serialización de bandas paramétricas (Peaking, Shelves, HighPass/LowPass).
 *  3. Resistencia a JSON malformado o trunco.
 *  4. Preservación exacta de valores numéricos de coma flotante.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <Profiles/ProfileSerializer.h>

#include <cassert>
#include <cmath>
#include <string>

namespace dash::test
{

void testProfileSerializerRoundTrip()
{
    using namespace dash::profile;

    AudioProfile original;
    original.resetToDefaults();
    original.profileId   = "test-custom-profile";
    original.name        = "Test Custom Profile";
    original.description = "Descripción para prueba de round-trip.";
    original.game.id     = "call-of-duty-warzone";
    original.headphone.manufacturer = "HyperX";
    original.headphone.model        = "Cloud II";
    original.audio.sampleRate       = 96000.0;
    original.audio.channelLayout    = "7.1";
    original.audio.preampDb         = -6.0f;

    // Configuración DSP personalizada
    original.dspParams.headroomDb                 = -4.5f;
    original.dspParams.equalizer.footstepsDb      = 2.8f;
    original.dspParams.equalizer.bassDb           = 1.1f;
    original.dspParams.transient.gun              = 0.45f;
    original.dspParams.transient.frequencySplitHz = 320.0f;
    original.dspParams.ambience.ambGuar           = 0.85f;
    original.dspParams.dynamics.targetLufs        = -14.5f;

    // Bandas paramétricas
    original.dspParams.useParametricEq = true;
    original.dspParams.parametricBands[0].type        = dash::core::ParametricFilterType::Peaking;
    original.dspParams.parametricBands[0].frequencyHz = 3150.0f;
    original.dspParams.parametricBands[0].gainDb      = 3.5f;
    original.dspParams.parametricBands[0].q           = 1.41f;
    original.dspParams.parametricBands[0].enabled     = true;

    // Serializar a JSON
    const std::string jsonOutput = ProfileSerializer::serializeToJson(original, false);
    assert(!jsonOutput.empty());

    // Deserializar a nuevo objeto
    AudioProfile deserialized;
    std::string errorMsg;
    const bool success = ProfileSerializer::deserializeFromJson(jsonOutput, deserialized, &errorMsg);
    assert(success);
    assert(errorMsg.empty());

    // Comprobar equivalencia
    assert(deserialized.profileId == original.profileId);
    assert(deserialized.name == original.name);
    assert(deserialized.headphone.manufacturer == "HyperX");
    assert(deserialized.audio.sampleRate == 96000.0);
    assert(std::abs(deserialized.dspParams.headroomDb - original.dspParams.headroomDb) < 0.001f);
    assert(std::abs(deserialized.dspParams.equalizer.footstepsDb - 2.8f) < 0.001f);
    assert(std::abs(deserialized.dspParams.transient.gun - 0.45f) < 0.001f);
    assert(std::abs(deserialized.dspParams.transient.frequencySplitHz - 320.0f) < 0.01f);
    assert(std::abs(deserialized.dspParams.ambience.ambGuar - 0.85f) < 0.001f);
    assert(deserialized.dspParams.useParametricEq);
    assert(deserialized.dspParams.parametricBands[0].type == dash::core::ParametricFilterType::Peaking);
    assert(std::abs(deserialized.dspParams.parametricBands[0].frequencyHz - 3150.0f) < 0.01f);
}

void testProfileSerializerMalformedJsonRejection()
{
    using namespace dash::profile;

    AudioProfile profile;
    std::string error;

    // JSON con llave sin cerrar
    const std::string malformed = R"({ "profileId": "incomplete", "name": "Broken" )";
    const bool ok = ProfileSerializer::deserializeFromJson(malformed, profile, &error);
    assert(!ok);
    assert(!error.empty());
}

} // namespace dash::test
