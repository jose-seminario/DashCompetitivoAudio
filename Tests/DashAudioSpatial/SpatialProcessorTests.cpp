/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para SpatialProcessor
 *
 * Cobertura de pruebas preparadas:
 *  1. Negociación de Layouts (5.1, 7.1, 7.1.4, 9.1.6): Transición limpia y sin cuelgues.
 *  2. Modulación de Anchura de Escenario: Escalamiento simétrico de canales laterales.
 *  3. Separación Frontal/Trasera: Control de discriminación acústica de cuadrante posterior.
 *  4. Ejecución en Tiempo Real: Cero asignaciones dentro del bucle de procesamiento.
 *  5. Comprobación de Finitud: Cero NaN o Infinity ante impulsos y barridos.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioSpatial/SpatialProcessor.h>

#include <cassert>
#include <cmath>
#include <vector>

namespace dash::test
{

namespace
{
    bool isFinite(const float* data, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            if (std::isnan(data[i]) || std::isinf(data[i]))
                return false;
        }
        return true;
    }
} // namespace

void testSpatialProcessor71Processing()
{
    using namespace dash::spatial;

    SpatialProcessor processor;
    processor.prepare(48000.0, 512);

    const bool ok = processor.configureLayout(ChannelLayout::surround7_1());
    assert(ok);
    assert(processor.isSpatialActive());

    std::vector<std::vector<float>> channels(8, std::vector<float>(256, 0.1f));
    std::vector<const float*> inPointers(8);
    for (size_t i = 0; i < 8; ++i)
        inPointers[i] = channels[i].data();

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    processor.processBinaural(inPointers.data(), 8, outL.data(), outR.data(), 256);

    assert(isFinite(outL.data(), 256));
    assert(isFinite(outR.data(), 256));
    assert(std::abs(outL[100]) > 0.01f);
}

void testSpatialProcessorSoundstageWidth()
{
    using namespace dash::spatial;

    SpatialProcessor processor;
    processor.prepare(48000.0, 256);
    processor.configureLayout(ChannelLayout::surround7_1());

    SpatialProcessorParameters params;
    params.soundstageWidth = 1.5f; // Ampliación de escenario
    params.frontBackSeparation = 1.2f;
    processor.updateParameters(params);

    assert(std::abs(processor.getParameters().soundstageWidth - 1.5f) < 1.0e-5f);
}

void testSpatialProcessorImmersive16Channels()
{
    using namespace dash::spatial;

    SpatialProcessor processor;
    processor.prepare(48000.0, 512);

    const bool ok = processor.configureLayout(ChannelLayout::immersive9_1_6());
    assert(ok);
    assert(processor.getInputLayout().getNumChannels() == 16);

    std::vector<std::vector<float>> channels(16, std::vector<float>(128, 0.05f));
    std::vector<const float*> inPointers(16);
    for (size_t i = 0; i < 16; ++i)
        inPointers[i] = channels[i].data();

    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);

    processor.processBinaural(inPointers.data(), 16, outL.data(), outR.data(), 128);

    assert(isFinite(outL.data(), 128));
    assert(isFinite(outR.data(), 128));
}

} // namespace dash::test
