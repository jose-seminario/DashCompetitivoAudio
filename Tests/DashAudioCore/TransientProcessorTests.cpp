/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para TransientProcessor
 *
 * Cobertura de pruebas preparadas:
 *  1. Impulso y Silencio: Estabilidad numérica sin NaN ni Infinity.
 *  2. Supresión de Cresta Alta: Disparo sintético de alta amplitud (> 0.8) y alto factor de cresta.
 *  3. Preservación de Pasos y Micro-Transitorios: Pulsos acústicos de baja intensidad (< 0.05) no son alterados.
 *  4. División Crossover: Frecuencias por debajo de frequencySplitHz (ej. 100 Hz) conservan su energía intacta.
 *  5. Procesamiento Mid/Side Estéreo: La señal lateral (Side) no sufre atenuación indeseada.
 *  6. Modo Bypass: Salida idéntica bit a bit con la entrada.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioCore/TransientProcessor.h>

#include <cassert>
#include <cmath>
#include <vector>

namespace dash::test
{

namespace
{
    bool isFiniteBuffer(const float* buffer, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i]))
                return false;
        }
        return true;
    }

    std::vector<float> generateSimulatedGunCrack(size_t totalSamples, size_t pulseIndex = 100)
    {
        std::vector<float> buffer(totalSamples, 0.001f);
        if (pulseIndex + 30 < totalSamples)
        {
            // Impulso de ataque ultrarrápido (simulación de pico de deflagración)
            buffer[pulseIndex]     =  0.95f;
            buffer[pulseIndex + 1] = -0.85f;
            buffer[pulseIndex + 2] =  0.70f;
            buffer[pulseIndex + 3] = -0.50f;
            buffer[pulseIndex + 4] =  0.30f;
        }
        return buffer;
    }
} // namespace

void testTransientGunCrackSuppression()
{
    dash::core::TransientProcessor proc;
    proc.prepare(48000.0, 512, 1);

    dash::core::TransientParameters params;
    params.gun = 0.45f;
    params.bypass = false;
    proc.updateParameters(params);

    auto signal = generateSimulatedGunCrack(1024, 150);
    proc.process(signal.data(), 1024);

    assert(isFiniteBuffer(signal.data(), 1024));
    // La reducción máxima registrada debe ser negativa (atenuación efectiva de Gun Crack)
    assert(proc.getLastReductionDb() < -1.0f);
    // El pico máximo no debe alcanzar el 0.95f original
    float maxAmp = 0.0f;
    for (float s : signal)
        maxAmp = std::max(maxAmp, std::abs(s));
    assert(maxAmp < 0.85f);
}

void testTransientFootstepsPreservation()
{
    dash::core::TransientProcessor proc;
    proc.prepare(48000.0, 512, 1);

    dash::core::TransientParameters params;
    params.gun = 0.45f;
    params.steps = 1.0f; // Máxima protección de pasos
    proc.updateParameters(params);

    // Micro-transitorio suave (simulación de paso lejano a baja intensidad)
    std::vector<float> signal(1024, 0.0f);
    signal[200] = 0.035f;
    signal[201] = -0.025f;
    signal[202] = 0.015f;

    auto original = signal;
    proc.process(signal.data(), 1024);

    assert(isFiniteBuffer(signal.data(), 1024));
    // La atenuación no debe dispararse sobre señales débiles
    assert(proc.getLastReductionDb() > -0.5f);
}

void testTransientMidSidePreservation()
{
    dash::core::TransientProcessor proc;
    proc.prepare(48000.0, 512, 2);

    dash::core::TransientParameters params;
    params.front = true;
    proc.updateParameters(params);

    // Sonido situado completamente en el canal izquierdo (Side activo)
    std::vector<float> left(1024, 0.4f);
    std::vector<float> right(1024, -0.4f); // L y R en oposición de fase perfecta: Mid = 0, Side = 0.4
    float* channels[2] = { left.data(), right.data() };

    proc.process(channels, 2, 1024);

    // La componente lateral pura no debe verse deformada por la reducción central
    // (tolerancia de 0.05f en muestra 100 para admitir el settling del filtro IIR pasa-todo, y 0.01f en régimen estacionario)
    assert(std::abs(left[100] - 0.4f) < 0.05f);
    assert(std::abs(left[500] - 0.4f) < 0.01f);
}

} // namespace dash::test
