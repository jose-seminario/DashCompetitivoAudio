#pragma once

#include <cstddef>
#include <vector>

namespace dash::bench
{

struct LatencyReport
{
    float bufferDurationMs     = 0.0f;
    float algorithmicDelayMs   = 0.0f;
    float roundTripEstimateMs  = 0.0f;
    float measuredItdUs        = 0.0f; // Interaural Time Difference en microsegundos
    bool  isZeroLatencyStage   = true;
};

/**
 * Analizador de Latencia Algorítmica y Retardos Temporales (LatencyAnalyzer).
 *
 * Mide el retardo de búfer y calcula el desfase temporal interaural (ITD)
 * mediante correlación cruzada entre canales izquierdo y derecho.
 */
class LatencyAnalyzer
{
public:
    static float calculateBufferDurationMs(int bufferSize, double sampleRate) noexcept;
    static float measureCrossCorrelationLagSamples(const float* sigA, const float* sigB, size_t numSamples, size_t maxLag) noexcept;
    static LatencyReport evaluateChainLatency(int bufferSize, double sampleRate, size_t firFilterTaps = 0) noexcept;
};

} // namespace dash::bench
