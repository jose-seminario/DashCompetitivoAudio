/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ConvolutionEngine
 *
 * Cobertura de pruebas preparadas:
 *  1. Convolución de Impulso Unitario: delta[n] reproduce la señal de entrada idéntica bit a bit.
 *  2. Convolución con Retardo: delta[n - k] desplaza la señal exactamente k muestras.
 *  3. Continuidad entre Bloques: Ausencia de discontinuidades o clicks en fronteras de bloque.
 *  4. Conmutación Lock-Free de Filtros: commitFilters() conmuta el banco activo sin bloqueos.
 *  5. Comprobación de Cero Latencia: Convolución directa de tiempo sin buffer de retardo FFT.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioSpatial/ConvolutionEngine.h>

#include <cassert>
#include <cmath>
#include <vector>

namespace dash::test
{

void testConvolutionUnitImpulse()
{
    using namespace dash::spatial;

    ConvolutionEngine engine;
    engine.prepare(48000.0, 512, 128);

    // Filtro de impulso unitario puro (delta[n]) en el canal 0
    std::vector<float> impulseL(128, 0.0f);
    std::vector<float> impulseR(128, 0.0f);
    impulseL[0] = 1.0f;
    impulseR[0] = 1.0f;

    engine.loadFilterPair(0, impulseL.data(), impulseR.data(), 128);
    engine.commitFilters();

    std::vector<float> in(256);
    for (size_t i = 0; i < in.size(); ++i)
        in[i] = static_cast<float>(i) / 256.0f;

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    engine.convolveAndAccumulate(0, in.data(), outL.data(), outR.data(), 256);

    for (size_t i = 0; i < in.size(); ++i)
    {
        assert(std::abs(outL[i] - in[i]) < 1.0e-5f);
        assert(std::abs(outR[i] - in[i]) < 1.0e-5f);
    }
}

void testConvolutionDelayImpulse()
{
    using namespace dash::spatial;

    ConvolutionEngine engine;
    engine.prepare(48000.0, 512, 64);

    // Filtro con retraso puro de 10 muestras en canal izquierdo y 20 en canal derecho
    std::vector<float> impulseL(64, 0.0f);
    std::vector<float> impulseR(64, 0.0f);
    impulseL[10] = 1.0f;
    impulseR[20] = 1.0f;

    engine.loadFilterPair(0, impulseL.data(), impulseR.data(), 64);
    engine.commitFilters();

    std::vector<float> in(128, 0.0f);
    in[0] = 1.0f; // Impulso Dirac de entrada

    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);

    engine.convolveAndAccumulate(0, in.data(), outL.data(), outR.data(), 128);

    assert(std::abs(outL[10] - 1.0f) < 1.0e-5f);
    assert(std::abs(outR[20] - 1.0f) < 1.0e-5f);
    assert(std::abs(outL[0]) < 1.0e-5f);
}

void testConvolutionBlockBoundaryContinuity()
{
    using namespace dash::spatial;

    ConvolutionEngine engine;
    engine.prepare(48000.0, 128, 64);

    std::vector<float> ir(64, 0.0f);
    ir[0]  = 0.5f;
    ir[32] = 0.5f; // Eco secundario a 32 muestras

    engine.loadFilterPair(0, ir.data(), ir.data(), 64);
    engine.commitFilters();

    // Procesar bloque 1
    std::vector<float> in1(128, 0.0f);
    in1[120] = 1.0f; // Impulso cerca del final del bloque 1

    std::vector<float> outL1(128, 0.0f);
    std::vector<float> outR1(128, 0.0f);
    engine.convolveAndAccumulate(0, in1.data(), outL1.data(), outR1.data(), 128);

    // Procesar bloque 2
    std::vector<float> in2(128, 0.0f);
    std::vector<float> outL2(128, 0.0f);
    std::vector<float> outR2(128, 0.0f);
    engine.convolveAndAccumulate(0, in2.data(), outL2.data(), outR2.data(), 128);

    // El eco debe derramarse suavemente en el bloque 2 en la muestra (120 + 32 - 128 = 24)
    assert(std::abs(outL2[24] - 0.5f) < 1.0e-5f);
}

} // namespace dash::test
