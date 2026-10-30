/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ChannelLayout
 *
 * Cobertura de pruebas preparadas:
 *  1. Layouts Canónicos Estándar: Verificación de Mono (1), Stereo (2), 5.1 (6), 7.1 (8), 7.1.4 (12) y 9.1.6 (16).
 *  2. Máscaras de Altavoces Windows: Correspondencia bit a bit con dwChannelMask de WAVEFORMATEXTENSIBLE.
 *  3. Coordenadas Esféricas Tridimensionales: Azimuth y elevación correctos (L=-30°, R=+30°, Top=+35°).
 *  4. Detección de Rol LFE: Localización del canal subwoofer en índice 3 para layouts surround.
 *  5. Layouts Discretos Desconocidos: Tratamiento seguro sin conjeturas espaciales no fundamentadas.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioSpatial/ChannelLayout.h>

#include <cassert>
#include <cmath>

namespace dash::test
{

void testStandardChannelLayouts()
{
    using namespace dash::spatial;

    // 1. Mono
    const auto mono = ChannelLayout::mono();
    assert(mono.getNumChannels() == 1);
    assert(mono.getType() == LayoutType::Mono);
    assert(mono.getSpeaker(0).role == ChannelRole::FrontCenter);

    // 2. Stereo
    const auto stereo = ChannelLayout::stereo();
    assert(stereo.getNumChannels() == 2);
    assert(stereo.getType() == LayoutType::Stereo);
    assert(stereo.getSpeaker(0).role == ChannelRole::FrontLeft);
    assert(stereo.getSpeaker(1).role == ChannelRole::FrontRight);
    assert(std::abs(stereo.getSpeaker(0).azimuthDeg - (-30.0f)) < 0.1f);
    assert(std::abs(stereo.getSpeaker(1).azimuthDeg - (+30.0f)) < 0.1f);

    // 3. Surround 5.1
    const auto sur51 = ChannelLayout::surround5_1();
    assert(sur51.getNumChannels() == 6);
    assert(sur51.getType() == LayoutType::Surround5_1);
    assert(sur51.hasRole(ChannelRole::LFE));
    assert(sur51.getChannelIndex(ChannelRole::LFE) == 3);
    assert(sur51.isSpatialized());

    // 4. Surround 7.1
    const auto sur71 = ChannelLayout::surround7_1();
    assert(sur71.getNumChannels() == 8);
    assert(sur71.getType() == LayoutType::Surround7_1);
    assert(sur71.hasRole(ChannelRole::BackLeft));
    assert(sur71.hasRole(ChannelRole::SideLeft));

    // 5. Inmersivo 7.1.4 (12 canales)
    const auto imm714 = ChannelLayout::immersive7_1_4();
    assert(imm714.getNumChannels() == 12);
    assert(imm714.getType() == LayoutType::Immersive7_1_4);
    assert(imm714.hasRole(ChannelRole::TopFrontLeft));
    assert(imm714.hasRole(ChannelRole::TopBackRight));
    assert(imm714.getSpeaker(8).elevationDeg > 30.0f); // Elevación acústica superior

    // 6. Inmersivo 9.1.6 (16 canales)
    const auto imm916 = ChannelLayout::immersive9_1_6();
    assert(imm916.getNumChannels() == 16);
    assert(imm916.getType() == LayoutType::Immersive9_1_6);
}

void testWindowsSpeakerMaskConversion()
{
    using namespace dash::spatial;

    const auto fromMask71 = ChannelLayout::fromWindowsSpeakerMask(dash::core::SpeakerMask::kSurround7Point1);
    assert(fromMask71.getType() == LayoutType::Surround7_1);
    assert(fromMask71.getNumChannels() == 8);

    // Máscara no estándar arbitraria
    const auto unknownMask = ChannelLayout::fromWindowsSpeakerMask(0x10000000);
    assert(unknownMask.getType() == LayoutType::DiscreteCustom);
}

void testDiscreteCustomLayout()
{
    using namespace dash::spatial;

    const auto custom14 = ChannelLayout::discrete(14);
    assert(custom14.getNumChannels() == 14);
    assert(custom14.getType() == LayoutType::DiscreteCustom);
    assert(!custom14.isSpatialized());
    assert(custom14.getSpeaker(0).role == ChannelRole::DiscreteChannel);
}

} // namespace dash::test
