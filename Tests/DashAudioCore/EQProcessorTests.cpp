/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ParametricEQProcessor (6 Bandas Estéreo)
 */

#include <DashAudioCore/ParametricEQProcessor.h>

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

    std::vector<float> generateSine(float freqHz, double sampleRate, size_t numSamples, float amp = 0.25f)
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

void testSixBandDefaultConfiguration()
{
    using namespace dash::core;

    static_assert(ParametricEQProcessor::kMaxBands == 6);
    static_assert(ParametricEQProcessor::kMaxChannels == 2);

    ParametricEQProcessor eq;
    eq.prepare(48000.0, 512, 2);

    const auto b0 = eq.getBand(0);
    const auto b1 = eq.getBand(1);
    const auto b2 = eq.getBand(2);
    const auto b3 = eq.getBand(3);
    const auto b4 = eq.getBand(4);
    const auto b5 = eq.getBand(5);

    assert(b0.type == ParametricFilterType::LowShelf  && std::abs(b0.frequencyHz - 60.0f)   < 0.01f && std::abs(b0.q - 0.707f) < 0.01f);
    assert(b1.type == ParametricFilterType::LowShelf  && std::abs(b1.frequencyHz - 150.0f)  < 0.01f && std::abs(b1.q - 0.707f) < 0.01f);
    assert(b2.type == ParametricFilterType::Peaking   && std::abs(b2.frequencyHz - 400.0f)  < 0.01f && std::abs(b2.q - 1.0f)   < 0.01f);
    assert(b3.type == ParametricFilterType::Peaking   && std::abs(b3.frequencyHz - 1500.0f) < 0.01f && std::abs(b3.q - 1.2f)   < 0.01f);
    assert(b4.type == ParametricFilterType::Peaking   && std::abs(b4.frequencyHz - 3500.0f) < 0.01f && std::abs(b4.q - 1.2f)   < 0.01f);
    assert(b5.type == ParametricFilterType::HighShelf && std::abs(b5.frequencyHz - 8000.0f) < 0.01f && std::abs(b5.q - 0.707f) < 0.01f);
}

void testLowShelfAndPeakingBoost()
{
    dash::core::ParametricEQProcessor eq;
    eq.prepare(48000.0, 512, 2);

    // Banda 4 (Pasos Agudos: Peaking 3500 Hz) +6 dB
    eq.setBandGainDb(4, 6.0f);

    auto left  = generateSine(3500.0f, 48000.0, 4800, 0.2f);
    auto right = generateSine(3500.0f, 48000.0, 4800, 0.2f);
    const float inRms = computeRms(left.data() + 1000, left.size() - 1000);

    float* chs[2] = { left.data(), right.data() };
    eq.process(chs, 2, static_cast<int>(left.size()));

    assert(isFiniteBuffer(left.data(), left.size()));
    assert(isFiniteBuffer(right.data(), right.size()));

    const float outRmsL = computeRms(left.data() + 1000, left.size() - 1000);
    const float outRmsR = computeRms(right.data() + 1000, right.size() - 1000);
    assert(outRmsL > inRms * 1.7f);
    assert(outRmsR > inRms * 1.7f);
}

void testHighShelfAttenuation()
{
    dash::core::ParametricEQProcessor eq;
    eq.prepare(48000.0, 512, 2);

    // Banda 5 (Disparos: HighShelf 8000 Hz) -9 dB
    eq.setBandGainDb(5, -9.0f);

    auto highTone = generateSine(12000.0f, 48000.0, 4800, 0.4f);
    const float inRms = computeRms(highTone.data() + 1000, highTone.size() - 1000);

    eq.process(highTone.data(), static_cast<int>(highTone.size()));

    assert(isFiniteBuffer(highTone.data(), highTone.size()));
    const float outRms = computeRms(highTone.data() + 1000, highTone.size() - 1000);
    assert(outRms < inRms * 0.55f);
}

void testStereoChannelIsolation()
{
    dash::core::ParametricEQProcessor eq;
    eq.prepare(48000.0, 512, 2);
    eq.setBandGainDb(0, 4.0f);
    eq.setBandGainDb(3, 5.0f);

    auto left = generateSine(1500.0f, 48000.0, 2048, 0.3f);
    std::vector<float> right(2048, 0.0f);

    float* chs[2] = { left.data(), right.data() };
    eq.process(chs, 2, 2048);

    // Estéreo puro: el canal derecho en silencio no recibe crossfeed del izquierdo
    for (float s : right)
        assert(std::abs(s) < 1.0e-7f);

    assert(computeRms(left.data() + 256, left.size() - 256) > 0.25f);
}

} // namespace dash::test
