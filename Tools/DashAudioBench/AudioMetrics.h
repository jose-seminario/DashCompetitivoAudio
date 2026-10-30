#pragma once

#include <cstddef>
#include <vector>

namespace dash::bench
{

struct QualityMetrics
{
    float peakDbFs        = -100.0f;
    float rmsDbFs         = -100.0f;
    float lufsIntegrated  = -100.0f;
    float crestFactorDb   = 0.0f;
    float thdPercent      = 0.0f;
    float snrDb           = 0.0f;
    size_t clippedSamples = 0;
    bool  isClipped       = false;
};

/**
 * Calculador de Métricas de Calidad Acústica y Distorsión (AudioMetrics).
 *
 * Implementa medición rigurosa de niveles Peak, RMS, LUFS (ITU-R BS.1770-4)
 * y detección de saturación digital/clipping.
 */
class AudioMetrics
{
public:
    static float calculatePeakDbFs(const float* buffer, size_t numSamples) noexcept;
    static float calculateRmsDbFs(const float* buffer, size_t numSamples) noexcept;
    static float calculateLufs(const float* const* channelPointers, int numChannels, size_t numSamples, double sampleRate) noexcept;
    static size_t countClippingSamples(const float* buffer, size_t numSamples, float threshold = 0.9999f) noexcept;
    static float calculateTHD(const float* buffer, size_t numSamples, double sampleRate, float testFreqHz) noexcept;

    static QualityMetrics analyzeBuffer(const float* buffer, size_t numSamples, double sampleRate) noexcept;
};

} // namespace dash::bench
