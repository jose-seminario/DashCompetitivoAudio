#pragma once

#include "AudioMetrics.h"

#include <cstddef>
#include <vector>

namespace dash::bench
{

struct ABComparisonResult
{
    QualityMetrics metricsA; // Original
    QualityMetrics metricsB; // Procesado (sin compensar)
    QualityMetrics metricsBMatched; // Procesado con sonoridad compensada (Loudness-Matched)

    float gainCompensationAppliedDb = 0.0f;
    float dynamicRangeReductionDb   = 0.0f;
    float footstepsBoostDb          = 0.0f;
    float gunCrackAttenuationDb     = 0.0f;
    float correlationCoefficient    = 0.0f;
};

/**
 * Motor de Comparación y A/B Testing con Compensación de Sonoridad (ABComparison).
 *
 * Implementa normalización automática de LUFS/RMS entre la señal original y la procesada
 * para erradicar el sesgo psicoacústico ("más fuerte suena mejor") y permitir evaluar
 * la inteligibilidad táctica real de los algoritmos DSP.
 */
class ABComparison
{
public:
    /**
     * Aplica compensación de sonoridad a una copia del buffer B para igualar los LUFS del buffer A.
     */
    static std::vector<float> matchLoudness(
        const float* bufferA,
        const float* bufferB,
        size_t numSamples,
        double sampleRate,
        float* outAppliedGainDb = nullptr);

    /**
     * Realiza la evaluación comparativa completa A/B.
     */
    static ABComparisonResult compare(
        const float* originalA,
        const float* processedB,
        size_t numSamples,
        double sampleRate);
};

} // namespace dash::bench
