/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para AudioGraph (Estéreo 6 Bandas ±18 dB + Limitador)
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

    std::vector<float> generateSineWave(float freqHz, double sampleRate, size_t numSamples, float amp = 0.25f)
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

    float computeRms(const float* data, size_t size)
    {
        double sum = 0.0;
        for (size_t i = 0; i < size; ++i)
            sum += static_cast<double>(data[i] * data[i]);
        return static_cast<float>(std::sqrt(sum / static_cast<double>(size)));
    }
} // namespace

void testAudioGraphStereoSixBandProcessing()
{
    dash::core::AudioGraph graph;
    graph.prepare(48000.0, 512, 2);

    dash::core::AudioGraphParameters params;
    params.preampDb = 0.0f;
    params.bandGainsDb = { -6.0f, -3.0f, 0.0f, 9.0f, 12.0f, -6.0f };
    params.bypassAll = false;
    graph.updateParameters(params);

    // Tono en la banda de Pasos Medios (1500 Hz) debe realzarse limpiamente en estéreo
    auto left  = generateSineWave(1500.0f, 48000.0, 2048, 0.15f);
    auto right = generateSineWave(1500.0f, 48000.0, 2048, 0.15f);
    const float inRms = computeRms(left.data() + 512, left.size() - 512);

    float* channels[2] = { left.data(), right.data() };
    graph.process(channels, 2, 2048);

    assert(isFiniteBuffer(left.data(), 2048));
    assert(isFiniteBuffer(right.data(), 2048));

    const float outRmsL = computeRms(left.data() + 512, left.size() - 512);
    const float outRmsR = computeRms(right.data() + 512, right.size() - 512);
    assert(outRmsL > inRms * 2.0f);
    assert(outRmsR > inRms * 2.0f);
}

void testAudioGraphStereoChannelIsolation()
{
    dash::core::AudioGraph graph;
    graph.prepare(48000.0, 512, 2);
    graph.setBandGainDb(4, 15.0f); // Pasos Agudos 3500 Hz

    auto left = generateSineWave(3500.0f, 48000.0, 1024, 0.15f);
    std::vector<float> right(1024, 0.0f);
    float* channels[2] = { left.data(), right.data() };

    graph.process(channels, 2, 1024);

    // Estéreo nativo puro: el canal opuesto permanece 100% independiente
    for (float s : right)
        assert(std::abs(s) < 1.0e-7f);
}

void testAudioGraphLimiterProtection()
{
    dash::core::AudioGraph graph;
    graph.prepare(48000.0, 512, 2);

    // Ajuste extremo: Preamp +18 dB y Banda 3 +18 dB (+36 dB combinados)
    graph.setPreampDb(18.0f);
    graph.setBandGainDb(3, 18.0f);
    assert(std::abs(graph.getParameters().preampDb - 18.0f) < 0.01f);
    assert(std::abs(graph.getParameters().bandGainsDb[3] - 18.0f) < 0.01f);

    auto left  = generateSineWave(1500.0f, 48000.0, 2048, 0.8f);
    auto right = generateSineWave(1500.0f, 48000.0, 2048, 0.8f);
    float* channels[2] = { left.data(), right.data() };

    graph.process(channels, 2, 2048);

    // El LimiterProcessor (-0.5 dBFS ~= 0.944061 lineal) frena toda sobrecarga al final de la cadena
    constexpr float kCeilingLin = 0.944062f;
    for (size_t i = 0; i < 2048; ++i)
    {
        assert(std::abs(left[i])  <= kCeilingLin + 1.0e-5f);
        assert(std::abs(right[i]) <= kCeilingLin + 1.0e-5f);
    }
    assert(graph.getLimiterReductionDb() < -15.0f);
}

void testAudioGraphBypassAll()
{
    dash::core::AudioGraph graph;
    graph.prepare(48000.0, 512, 2);
    graph.setPreampDb(12.0f);
    graph.setBandGainDb(0, 12.0f);
    graph.setBypass(true);

    auto left  = generateSineWave(1000.0f, 48000.0, 1024, 0.7f);
    auto right = generateSineWave(2000.0f, 48000.0, 1024, 0.5f);
    const auto copyL = left;
    const auto copyR = right;

    float* channels[2] = { left.data(), right.data() };
    graph.process(channels, 2, 1024);

    for (size_t i = 0; i < 1024; ++i)
    {
        assert(std::abs(left[i]  - copyL[i]) < 1.0e-7f);
        assert(std::abs(right[i] - copyR[i]) < 1.0e-7f);
    }
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
            graph.setBandGainDb(1, 3.0f);
            graph.setBandGainDb(4, 4.0f);

            std::vector<float> l(static_cast<size_t>(bs), 0.1f);
            std::vector<float> r(static_cast<size_t>(bs), 0.1f);
            float* chs[2] = { l.data(), r.data() };

            graph.process(chs, 2, bs);

            assert(isFiniteBuffer(l.data(), static_cast<size_t>(bs)));
            assert(isFiniteBuffer(r.data(), static_cast<size_t>(bs)));
        }
    }
}

} // namespace dash::test
