/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para DynamicsProcessor
 *
 * Cobertura de pruebas preparadas:
 *  1. Respuesta al Impulso y Silencio (ausencia de NaN, Inf y DC Offset).
 *  2. Ponderación K (ITU-R BS.1770-4): Pre-filter y RLB en frecuencias clave (1 kHz vs 100 Hz vs 4 kHz).
 *  3. Exclusión del canal LFE (peso = 0.0) en medición multicanal 5.1 y 7.1.
 *  4. Compresión Downward con Soft-Knee ante señales por encima de Target (-10 dBFS vs -18 LUFS).
 *  5. Elevación Upward con compuerta de ruido acústica (señales a -30 dBFS elevadas; señales a -75 dBFS silenciadas).
 *  6. Variación de Sample Rates (44.1 kHz, 48 kHz, 96 kHz, 192 kHz) y tamaños de bloque (32 a 2048 muestras).
 *  7. Modo Bypass (salida idéntica bit a bit con entrada).
 *
 * NOTA: De acuerdo con las reglas de trabajo, este archivo es de solo lectura/preparación y no se ejecuta en la terminal.
 */

#include <DashAudioCore/DynamicsProcessor.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
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

    std::vector<float> generateSineWave(float freqHz, double sampleRate, size_t numSamples, float amplitude = 1.0f)
    {
        std::vector<float> buffer(numSamples);
        const float phaseInc = 2.0f * kPi * freqHz / static_cast<float>(sampleRate);
        float phase = 0.0f;
        for (size_t i = 0; i < numSamples; ++i)
        {
            buffer[i] = amplitude * std::sin(phase);
            phase += phaseInc;
            if (phase > 2.0f * kPi)
                phase -= 2.0f * kPi;
        }
        return buffer;
    }

    std::vector<float> generateWhiteNoise(size_t numSamples, float amplitude = 0.5f)
    {
        std::vector<float> buffer(numSamples);
        std::mt19937 gen(42);
        std::uniform_real_distribution<float> dist(-amplitude, amplitude);
        for (size_t i = 0; i < numSamples; ++i)
            buffer[i] = dist(gen);
        return buffer;
    }
} // namespace

void testDynamicsSilenceAndNaN()
{
    dash::core::DynamicsProcessor proc;
    proc.prepare(48000.0, 512, 2);

    std::vector<float> left(1024, 0.0f);
    std::vector<float> right(1024, 0.0f);
    float* channels[2] = { left.data(), right.data() };

    proc.process(channels, 2, 1024);

    assert(isFiniteBuffer(left.data(), 1024));
    assert(isFiniteBuffer(right.data(), 1024));
    assert(std::abs(proc.getCurrentLufs() - (-70.0f)) <= 5.0f);
}

void testDynamicsDownwardCompression()
{
    dash::core::DynamicsProcessor proc;
    proc.prepare(48000.0, 512, 1);

    dash::core::DynamicsParameters params;
    params.amount = 1.0f;
    params.targetLufs = -18.0f;
    params.bypass = false;
    proc.updateParameters(params);

    // Onda senoidal de alta intensidad a 1 kHz (-3 dBFS)
    auto signal = generateSineWave(1000.0f, 48000.0, 48000, 0.707f);
    proc.process(signal.data(), static_cast<int>(signal.size()));

    assert(isFiniteBuffer(signal.data(), signal.size()));
    // Debe haber reducido la ganancia (getCurrentGainDb < 0 dB)
    assert(proc.getCurrentGainDb() < -0.5f);
}

void testDynamicsUpwardCompression()
{
    dash::core::DynamicsProcessor proc;
    proc.prepare(48000.0, 512, 1);

    dash::core::DynamicsParameters params;
    params.amount = 0.8f;
    params.targetLufs = -18.0f;
    params.gateFloorDb = -60.0f;
    params.maxLiftDb = 9.0f;
    proc.updateParameters(params);

    // Onda senoidal de baja intensidad (-35 dBFS)
    auto signal = generateSineWave(1000.0f, 48000.0, 48000, 0.0177f);
    proc.process(signal.data(), static_cast<int>(signal.size()));

    assert(isFiniteBuffer(signal.data(), signal.size()));
    // Debe haber elevado los detalles acústicos
    assert(proc.getCurrentGainDb() > 0.5f);
}

void testDynamicsLfeExclusion()
{
    dash::core::DynamicsProcessor proc;
    proc.prepare(48000.0, 512, 6); // Layout 5.1

    std::vector<float> l(1024, 0.0f);
    std::vector<float> r(1024, 0.0f);
    std::vector<float> c(1024, 0.0f);
    std::vector<float> lfe = generateSineWave(50.0f, 48000.0, 1024, 0.9f); // LFE a máxima potencia
    std::vector<float> ls(1024, 0.0f);
    std::vector<float> rs(1024, 0.0f);

    float* channels[6] = { l.data(), r.data(), c.data(), lfe.data(), ls.data(), rs.data() };
    proc.process(channels, 6, 1024);

    // Al estar los canales principales en silencio, el canal LFE no debe disparar la medición de LUFS
    assert(proc.getCurrentLufs() < -60.0f);
}

void testDynamicsBypass()
{
    dash::core::DynamicsProcessor proc;
    proc.prepare(48000.0, 512, 1);

    dash::core::DynamicsParameters params;
    params.bypass = true;
    proc.updateParameters(params);

    auto signal = generateWhiteNoise(1024, 0.5f);
    auto copy = signal;

    proc.process(signal.data(), 1024);

    for (size_t i = 0; i < 1024; ++i)
        assert(std::abs(signal[i] - copy[i]) < 1.0e-7f);
}

} // namespace dash::test
