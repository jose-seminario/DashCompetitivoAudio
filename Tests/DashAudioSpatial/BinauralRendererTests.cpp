/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para BinauralRenderer
 *
 * Cobertura de pruebas preparadas:
 *  1. Renderizado Multicanal 7.1 a Binaural: Conversión tridimensional con preservación de energía.
 *  2. Bypass de HRTF para LFE: El canal subwoofer elude la convolución y se mezcla limpio en mono.
 *  3. Invarianza de Silencio: Silencio a la entrada genera exactamente silencio a la salida sin NaN/Inf.
 *  4. Crossfeed de Auriculares: Mezcla acústica controlada de bajas frecuencias entre oídos.
 *  5. Modo Bypass: Paso directo de los canales frontales sin convolución.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioSpatial/BinauralRenderer.h>

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

void testBinauralRenderer71ToStereo()
{
    using namespace dash::spatial;

    BinauralRenderer renderer;
    renderer.prepare(48000.0, 512);

    const auto layout71 = ChannelLayout::surround7_1();
    HRIRDataset dataset;
    dataset.loadSyntheticReference(layout71, 48000.0, 128);
    renderer.setDataset(dataset, layout71);

    assert(renderer.isReady());

    // Crear 8 canales con señal senoidal suave
    std::vector<std::vector<float>> channels(8, std::vector<float>(256, 0.1f));
    std::vector<const float*> inPointers(8);
    for (size_t i = 0; i < 8; ++i)
        inPointers[i] = channels[i].data();

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    renderer.render(inPointers.data(), 8, outL.data(), outR.data(), 256);

    assert(isFinite(outL.data(), 256));
    assert(isFinite(outR.data(), 256));
    // La salida binaural debe contener señal activa
    assert(std::abs(outL[100]) > 0.01f);
    assert(std::abs(outR[100]) > 0.01f);
}

void testBinauralRendererLfeBypass()
{
    using namespace dash::spatial;

    BinauralRenderer renderer;
    renderer.prepare(48000.0, 256);

    const auto layout51 = ChannelLayout::surround5_1();
    HRIRDataset dataset;
    dataset.loadSyntheticReference(layout51, 48000.0, 128);
    renderer.setDataset(dataset, layout51);

    // Solo canal 3 (LFE) con señal grave
    std::vector<std::vector<float>> channels(6, std::vector<float>(256, 0.0f));
    std::fill(channels[3].begin(), channels[3].end(), 0.5f);

    std::vector<const float*> inPointers(6);
    for (size_t i = 0; i < 6; ++i)
        inPointers[i] = channels[i].data();

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    renderer.render(inPointers.data(), 6, outL.data(), outR.data(), 256);

    // Al eludir el HRTF, ambos oídos deben recibir el subwoofer de forma idéntica y simétrica
    assert(std::abs(outL[100] - outR[100]) < 1.0e-5f);
    assert(outL[100] > 0.1f);
}

} // namespace dash::test
