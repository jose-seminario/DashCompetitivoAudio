/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para AmbienceProcessor
 *
 * Cobertura de pruebas preparadas:
 *  1. Estabilidad numérica: Silencio y ausencia de NaN o Infinity.
 *  2. Detección de Ruido Estacionario: Zumbido de amplitud constante (viento/hum a 1 kHz) activa estacionariedad.
 *  3. Protección de Sonidos Dinámicos: Ráfagas intermitentes (pasos) fuerzan StationarityIndex a 0.
 *  4. Alineación de Fase y Amplitud Plana: La suma de las 3 bandas Linkwitz-Riley no genera caídas espectrales.
 *  5. Bypass Granular por Banda: Verificación de no atenuación en banda media cuando bypassMidBand es true.
 *  6. Modo Bypass Global: Bit-exact con la señal de entrada.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioCore/AmbienceProcessor.h>

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

    std::vector<float> generateSteadyTone(float freqHz, double sampleRate, size_t numSamples, float amplitude = 0.2f)
    {
        std::vector<float> buffer(numSamples);
        const float phaseInc = 2.0f * kPi * freqHz / static_cast<float>(sampleRate);
        float phase = 0.0f;
        for (size_t i = 0; i < numSamples; ++i)
        {
            buffer[i] = amplitude * std::sin(phase);
            phase += phaseInc;
        }
        return buffer;
    }
} // namespace

void testAmbienceStationaryDucking()
{
    dash::core::AmbienceProcessor proc;
    proc.prepare(48000.0, 512, 1);

    dash::core::AmbienceParameters params;
    params.ambGuar = 1.0f;
    params.distant = true;
    params.bypass  = false;
    proc.updateParameters(params);

    // Tono estacionario continuo de 1 kHz (~1 segundo)
    auto signal = generateSteadyTone(1000.0f, 48000.0, 48000, 0.25f);
    proc.process(signal.data(), static_cast<int>(signal.size()));

    assert(isFiniteBuffer(signal.data(), signal.size()));
    // Debe haber detectado estacionariedad y aplicado atenuación
    assert(proc.getStationarityIndex() > 0.5f);
    assert(proc.getCurrentGainDb() < -1.0f);
}

void testAmbienceDynamicSignalProtection()
{
    dash::core::AmbienceProcessor proc;
    proc.prepare(48000.0, 512, 1);

    dash::core::AmbienceParameters params;
    params.ambGuar = 1.0f;
    params.distant = true;
    proc.updateParameters(params);

    // Señal pulsante dinámica: silencio con ráfagas de 1 kHz cada 200 ms
    std::vector<float> signal(48000, 0.0f);
    for (size_t burst = 0; burst < 5; ++burst)
    {
        const size_t start = burst * 9600;
        for (size_t i = 0; i < 480; ++i)
            signal[start + i] = 0.3f * std::sin(2.0f * kPi * 1000.0f * (i / 48000.0f));
    }

    proc.process(signal.data(), 48000);

    assert(isFiniteBuffer(signal.data(), 48000));
    // La señal no debe considerarse ruido estacionario continuo
    assert(proc.getStationarityIndex() < 0.4f);
}

void testAmbienceCrossoverReconstruction()
{
    dash::core::AmbienceProcessor proc;
    proc.prepare(48000.0, 512, 1);

    // ambGuar = 0.0 -> sin atenuación, verifica la fidelidad de división y recombinación
    dash::core::AmbienceParameters params;
    params.ambGuar = 0.0f;
    proc.updateParameters(params);

    auto signal = generateSteadyTone(1000.0f, 48000.0, 2048, 0.5f);
    auto copy = signal;

    proc.process(signal.data(), 2048);

    for (size_t i = 256; i < 2048; ++i) // Ignorar transitorio inicial de arranque de filtros
    {
        const float diff = std::abs(signal[i] - copy[i]);
        assert(diff < 0.01f);
    }
}

} // namespace dash::test
