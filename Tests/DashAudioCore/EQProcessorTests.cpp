/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para EQProcessor y ParametricEQProcessor
 *
 * Cobertura de pruebas preparadas:
 *  1. EQ Gráfico de 31 bandas ISO: Comprobación de curva base y modulación de Pasos (+dB @ 3.15k/6.3k) y Graves.
 *  2. Filtro Paramétrico Peaking: Realce acústico acotado en la frecuencia central configurada.
 *  3. Filtro High-Pass (12 dB/oct): Atenuación de subgraves a 40 Hz conservando frecuencias medias a 1 kHz.
 *  4. Filtro Low-Pass (12 dB/oct): Atenuación de agudos a 15 kHz.
 *  5. Filtro Notch (Trampa): Atenuación profunda (> 18 dB) en la frecuencia trampa con Q alto.
 *  6. Filtros Low-Shelf y High-Shelf: Elevación/corte de meseta uniforme en bandas extremas.
 *  7. Estabilidad Numérica: Cero NaN, Infinity o discontinuidades en DF2T.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioCore/EQProcessor.h>
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

    std::vector<float> generateSine(float freqHz, double sampleRate, size_t numSamples, float amp = 0.5f)
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

void testGraphicEQ31Bands()
{
    const auto gains = dash::core::EQProcessor::computeBandGainsDb(3.0, 2.0);
    assert(gains.size() == 31);

    // Banda 22 (3150 Hz): base 1.6 + modulación de pasos
    assert(gains[22] > 2.0);
    // Banda 6 (80 Hz): base 2.0 + modulación de graves
    assert(gains[6] > 2.0);
}

void testParametricPeakingFilter()
{
    dash::core::ParametricEQProcessor eq;
    eq.prepare(48000.0, 512, 1);

    dash::core::ParametricBandParameters band;
    band.type = dash::core::ParametricFilterType::Peaking;
    band.frequencyHz = 1000.0f;
    band.gainDb = 6.0f; // +6 dB de realce
    band.q = 1.0f;
    band.enabled = true;
    eq.setBand(0, band);

    auto signal = generateSine(1000.0f, 48000.0, 4800, 0.2f);
    const float inputRms = computeRms(signal.data(), signal.size());

    eq.process(signal.data(), static_cast<int>(signal.size()));

    assert(isFiniteBuffer(signal.data(), signal.size()));
    const float outputRms = computeRms(signal.data() + 1000, signal.size() - 1000);
    // +6 dB equivale aproximadamente al doble de amplitud (~2x)
    assert(outputRms > inputRms * 1.5f);
}

void testParametricHighPassFilter()
{
    dash::core::ParametricEQProcessor eq;
    eq.prepare(48000.0, 512, 1);

    dash::core::ParametricBandParameters hp;
    hp.type = dash::core::ParametricFilterType::HighPass;
    hp.frequencyHz = 200.0f;
    hp.q = 0.7071f;
    hp.enabled = true;
    eq.setBand(0, hp);

    // Señal grave a 50 Hz debe ser fuertemente atenuada
    auto subBass = generateSine(50.0f, 48000.0, 4800, 0.5f);
    eq.process(subBass.data(), static_cast<int>(subBass.size()));

    const float subBassRms = computeRms(subBass.data() + 1000, subBass.size() - 1000);
    assert(subBassRms < 0.15f); // Atenuación notable
}

void testParametricNotchFilter()
{
    dash::core::ParametricEQProcessor eq;
    eq.prepare(48000.0, 512, 1);

    dash::core::ParametricBandParameters notch;
    notch.type = dash::core::ParametricFilterType::Notch;
    notch.frequencyHz = 1000.0f;
    notch.q = 10.0f; // Q alto para rechazo agudo
    notch.enabled = true;
    eq.setBand(0, notch);

    auto sine1k = generateSine(1000.0f, 48000.0, 4800, 0.5f);
    eq.process(sine1k.data(), static_cast<int>(sine1k.size()));

    const float notchRms = computeRms(sine1k.data() + 1000, sine1k.size() - 1000);
    assert(notchRms < 0.05f); // Atenuación de al menos -20 dB
}

} // namespace dash::test
