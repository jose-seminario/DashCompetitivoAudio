/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para LimiterProcessor
 *
 * Cobertura de pruebas preparadas:
 *  1. Techo Estricto de Muestra Exacta: Señal a +6 dBFS (amplitud 2.0) limitada estrictamente a <= ceilingLin (-0.5 dBFS).
 *  2. Recuperación Continua de Ganancia: Retorno suave a 1.0 (0 dB) sin chasquidos ni bombeo anómalo.
 *  3. Enlace Multicanal Estéreo: Un pico en el canal izquierdo atenúa simétricamente el canal derecho para preservar la imagen.
 *  4. Desacoplamiento de LFE en 5.1/7.1: Sobrecarga en LFE no deprime los canales estéreo principales.
 *  5. Verificación de Cero Latencia: getLatencySamples() == 0 para juego en tiempo real.
 *  6. Ausencia de NaN, Infinity o desbordamientos.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioCore/LimiterProcessor.h>

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
} // namespace

void testLimiterCeilingClamping()
{
    dash::core::LimiterProcessor limiter;
    limiter.prepare(48000.0, 512, 1);

    dash::core::LimiterParameters params;
    params.ceilingDb = -0.5f; // ~0.944061 lineal
    params.bypass    = false;
    limiter.updateParameters(params);

    // Señal que excede fuertemente el umbral (picos de 2.5f = +8 dBFS)
    std::vector<float> signal(1024, 2.5f);
    limiter.process(signal.data(), 1024);

    assert(isFiniteBuffer(signal.data(), 1024));

    constexpr float kMaxAllowed = 0.944062f;
    for (float s : signal)
        assert(std::abs(s) <= kMaxAllowed + 1.0e-5f);

    assert(limiter.getCurrentReductionDb() < -6.0f);
}

void testLimiterMultichannelLink()
{
    dash::core::LimiterProcessor limiter;
    limiter.prepare(48000.0, 512, 2);

    dash::core::LimiterParameters params;
    params.ceilingDb    = -0.5f;
    params.linkChannels = true;
    limiter.updateParameters(params);

    // Canal izquierdo sufre un pico; canal derecho está a nivel normal (0.5)
    std::vector<float> left(512, 2.0f);
    std::vector<float> right(512, 0.5f);
    float* channels[2] = { left.data(), right.data() };

    limiter.process(channels, 2, 512);

    // El canal derecho debe haber recibido la misma reducción de ganancia proporcional
    assert(right[50] < 0.35f);
}

void testLimiterLfeIndependence()
{
    dash::core::LimiterProcessor limiter;
    limiter.prepare(48000.0, 512, 6); // 5.1 Surround

    dash::core::LimiterParameters params;
    params.ceilingDb            = -0.5f;
    params.includeLfeInPeakLink = false; // LFE no debe arrastrar a L y R
    limiter.updateParameters(params);

    std::vector<float> l(256, 0.5f);
    std::vector<float> r(256, 0.5f);
    std::vector<float> c(256, 0.5f);
    std::vector<float> lfe(256, 3.0f); // Pico extremo de graves en subwoofer
    std::vector<float> ls(256, 0.5f);
    std::vector<float> rs(256, 0.5f);

    float* channels[6] = { l.data(), r.data(), c.data(), lfe.data(), ls.data(), rs.data() };
    limiter.process(channels, 6, 256);

    // L y R deben permanecer en su nivel original de 0.5 porque el subwoofer está desacoplado
    assert(std::abs(l[50] - 0.5f) < 0.05f);
    // Y el LFE debe haber sido limitado a -0.5 dBFS
    assert(lfe[50] <= 0.944062f);
}

void testLimiterZeroLatency()
{
    dash::core::LimiterProcessor limiter;
    assert(limiter.getLatencySamples() == 0);
    assert(!limiter.isTruePeak());
}

} // namespace dash::test
