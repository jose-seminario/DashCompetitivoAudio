#include "LatencyAnalyzer.h"

#include <cmath>

namespace dash::bench
{

float LatencyAnalyzer::calculateBufferDurationMs(int bufferSize, double sampleRate) noexcept
{
    if (bufferSize <= 0 || sampleRate <= 0.0)
        return 0.0f;

    return static_cast<float>((static_cast<double>(bufferSize) / sampleRate) * 1000.0);
}

float LatencyAnalyzer::measureCrossCorrelationLagSamples(
    const float* sigA,
    const float* sigB,
    size_t numSamples,
    size_t maxLag) noexcept
{
    if (sigA == nullptr || sigB == nullptr || numSamples == 0 || maxLag == 0)
        return 0.0f;

    double maxCorrelation = -1e9;
    int bestLag = 0;

    const int iMaxLag = static_cast<int>(maxLag);

    for (int lag = -iMaxLag; lag <= iMaxLag; ++lag)
    {
        double sum = 0.0;
        size_t count = 0;

        for (size_t i = 0; i < numSamples; ++i)
        {
            const int idxB = static_cast<int>(i) + lag;
            if (idxB >= 0 && idxB < static_cast<int>(numSamples))
            {
                sum += static_cast<double>(sigA[i]) * static_cast<double>(sigB[idxB]);
                ++count;
            }
        }

        if (count > 0)
        {
            sum /= static_cast<double>(count);
            if (sum > maxCorrelation)
            {
                maxCorrelation = sum;
                bestLag = lag;
            }
        }
    }

    return static_cast<float>(bestLag);
}

LatencyReport LatencyAnalyzer::evaluateChainLatency(int bufferSize, double sampleRate, size_t firFilterTaps) noexcept
{
    LatencyReport r;
    r.bufferDurationMs = calculateBufferDurationMs(bufferSize, sampleRate);

    // En DashAudioCore, todos los procesadores (IIR DF2T, RMS, Linkwitz-Riley) tienen latencia algorítmica 0
    if (firFilterTaps > 0)
    {
        // Si se usa un filtro FIR de fase lineal, la latencia algorítmica es taps/2
        r.algorithmicDelayMs = static_cast<float>((static_cast<double>(firFilterTaps / 2) / sampleRate) * 1000.0);
        r.isZeroLatencyStage = false;
    }
    else
    {
        r.algorithmicDelayMs = 0.0f;
        r.isZeroLatencyStage = true;
    }

    r.roundTripEstimateMs = r.bufferDurationMs * 2.0f + r.algorithmicDelayMs;
    return r;
}

} // namespace dash::bench
