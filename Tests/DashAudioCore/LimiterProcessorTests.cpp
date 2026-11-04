/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para LimiterProcessor (Estéreo -0.5 dBFS)
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
    limiter.prepare(48000.0, 512, 2);

    dash::core::LimiterParameters params;
    params.ceilingDb = -0.5f; // ~0.944061 lineal
    params.bypass    = false;
    limiter.updateParameters(params);

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

    std::vector<float> left(512, 2.0f);
    std::vector<float> right(512, 0.5f);
    float* channels[2] = { left.data(), right.data() };

    limiter.process(channels, 2, 512);

    assert(right[50] < 0.35f);
}

void testLimiterZeroLatency()
{
    dash::core::LimiterProcessor limiter;
    assert(limiter.getLatencySamples() == 0);
    assert(!limiter.isTruePeak());
}

} // namespace dash::test
