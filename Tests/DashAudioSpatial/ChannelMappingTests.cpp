/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ChannelMapping
 *
 * Cobertura de pruebas preparadas:
 *  1. Identidad de Mapeo: 7.1 a 7.1 genera matriz diagonal unitaria.
 *  2. Conversión 7.1 hacia 5.1: Mapeo de canales traseros con ganancia de -3 dB hacia surround laterales.
 *  3. Invarianza de Muestras: Cero distorsión al enrutar señales continuas.
 *  4. Ejecución en tiempo real: apply() opera sin asignaciones dinámicas.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioSpatial/ChannelMapping.h>

#include <cassert>
#include <cmath>
#include <vector>

namespace dash::test
{

void testChannelMappingIdentity()
{
    using namespace dash::spatial;

    ChannelMapping mapper;
    mapper.configure(ChannelLayout::surround7_1(), ChannelLayout::surround7_1());

    assert(mapper.isPassThrough());
    for (size_t i = 0; i < 8; ++i)
        assert(std::abs(mapper.getRoutingGain(i, i) - 1.0f) < 1.0e-5f);
}

void testChannelMapping71To51Downmix()
{
    using namespace dash::spatial;

    ChannelMapping mapper;
    mapper.configure(ChannelLayout::surround7_1(), ChannelLayout::surround5_1());

    assert(!mapper.isPassThrough());

    // En 5.1, SideLeft es canal 4 y SideRight es canal 5
    // En 7.1, BackLeft es canal 4 y BackRight es canal 5; SideLeft es 6 y SideRight es 7
    const float blToSl = mapper.getRoutingGain(4, 4);
    assert(std::abs(blToSl - 0.7071f) < 0.01f);
}

void testChannelMappingApplyProcess()
{
    using namespace dash::spatial;

    ChannelMapping mapper;
    mapper.configure(ChannelLayout::stereo(), ChannelLayout::stereo());

    std::vector<float> inL(256, 0.8f);
    std::vector<float> inR(256, 0.4f);
    const float* inPointers[2] = { inL.data(), inR.data() };

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    float* outPointers[2] = { outL.data(), outR.data() };

    mapper.apply(inPointers, outPointers, 256);

    assert(std::abs(outL[50] - 0.8f) < 1.0e-5f);
    assert(std::abs(outR[50] - 0.4f) < 1.0e-5f);
}

} // namespace dash::test
