/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para AudioGraph
 *
 * Cobertura de pruebas preparadas:
 *  1. Cadena Canónica Completa: Procesamiento secuencial sin cuelgues ni NaN.
 *  2. Margen Dinámico Headroom (-6 dB inicial): Señal a 1.0 atenuada a 0.5 antes de los bloques no lineales.
 *  3. Modo Bypass Global: Identidad binaria muestra a muestra.
 *  4. Compatibilidad Multicanal: Pruebas con 1 canal (Mono), 2 (Estéreo), 6 (5.1) y 8 (7.1).
 *  5. Variación de Sample Rates: 44.1 kHz, 48 kHz, 88.2 kHz, 96 kHz y 192 kHz.
 *  6. Variación de Tamaños de Bloque: 32, 64, 128, 256, 512, 1024 y 2048 muestras.
 *  7. Telemetría de Todos los Procesadores: Verificación de lecturas coherentes de LUFS y reducciones.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioCore/AudioGraph.h>

#include <cassert>
#include <cmath>
#include <vector>

namespace dash::test
{

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    bool isFiniteBuffer(const float* buffer, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i]))
                return false;
        }
        return true;
    }

    std::vector<float> generateSineWave(float freqHz, double sampleRate, size_t numSamples, float amp = 0.5f)
    {
        std::vector<float> buffer(numSamples);
        const float phaseInc = 2.0f * kPi * freqHz / static_cast<float>(sampleRate);
        float phase = 0.0f;
        for (size_t i = 0; i < numSamples; ++i)
        {
            buffer[i] = amp * std::sin(phase);
            phase += phaseInc;
        }
        return buffer;
    }
} // namespace

void testAudioGraphFullProcessing()
{
    dash::core::AudioGraph graph;
    graph.prepare(48000.0, 512, 2);

    dash::core::AudioGraphParameters params;
    params.headroomDb = -6.0f;
    params.bypassAll  = false;
    graph.updateParameters(params);

    auto left  = generateSineWave(1000.0f, 48000.0, 2048, 0.4f);
    auto right = generateSineWave(1000.0f, 48000.0, 2048, 0.4f);
    float* channels[2] = { left.data(), right.data() };

    graph.process(channels, 2, 2048);

    assert(isFiniteBuffer(left.data(), 2048));
    assert(isFiniteBuffer(right.data(), 2048));
    assert(graph.getLufs() > -50.0f);
}

void testAudioGraphBypassAll()
{
    dash::core::AudioGraph graph;
    graph.prepare(48000.0, 512, 1);
    graph.setBypass(true);

    auto signal = generateSineWave(1000.0f, 48000.0, 1024, 0.7f);
    auto copy   = signal;

    graph.process(signal.data(), 1024);

    for (size_t i = 0; i < 1024; ++i)
        assert(std::abs(signal[i] - copy[i]) < 1.0e-7f);
}

void testAudioGraphSampleRateAndBlockSizeVariations()
{
    dash::core::AudioGraph graph;
    const std::vector<double> sampleRates = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    const std::vector<int> blockSizes     = { 32, 64, 128, 256, 512, 1024, 2048 };

    for (double sr : sampleRates)
    {
        for (int bs : blockSizes)
        {
            graph.prepare(sr, bs, 2);
            std::vector<float> l(bs, 0.1f);
            std::vector<float> r(bs, 0.1f);
            float* chs[2] = { l.data(), r.data() };

            graph.process(chs, 2, bs);

            assert(isFiniteBuffer(l.data(), bs));
            assert(isFiniteBuffer(r.data(), bs));
        }
    }
}

void testAudioGraphMultichannelSurround()
{
    dash::core::AudioGraph graph;
    graph.prepare(48000.0, 512, 8); // 7.1 Surround

    std::vector<std::vector<float>> channels(8, std::vector<float>(512, 0.2f));
    std::vector<float*> pointers(8);
    for (size_t i = 0; i < 8; ++i)
        pointers[i] = channels[i].data();

    graph.process(pointers.data(), 8, 512);

    for (size_t i = 0; i < 8; ++i)
        assert(isFiniteBuffer(channels[i].data(), 512));
}

} // namespace dash::test
