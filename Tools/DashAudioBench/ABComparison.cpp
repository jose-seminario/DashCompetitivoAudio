#include "ABComparison.h"
#include "FrequencyAnalyzer.h"

#include <cmath>

namespace dash::bench
{

std::vector<float> ABComparison::matchLoudness(
    const float* bufferA,
    const float* bufferB,
    size_t numSamples,
    double sampleRate,
    float* outAppliedGainDb)
{
    std::vector<float> matchedB(numSamples, 0.0f);
    if (bufferA == nullptr || bufferB == nullptr || numSamples == 0)
        return matchedB;

    const float lufsA = AudioMetrics::calculateRmsDbFs(bufferA, numSamples);
    const float lufsB = AudioMetrics::calculateRmsDbFs(bufferB, numSamples);

    float gainDb = 0.0f;
    if (lufsB > -100.0f && lufsA > -100.0f)
    {
        gainDb = lufsA - lufsB; // Compensación necesaria para igualar a A
    }

    if (outAppliedGainDb != nullptr)
        *outAppliedGainDb = gainDb;

    const float linearGain = std::pow(10.0f, gainDb / 20.0f);
    for (size_t i = 0; i < numSamples; ++i)
    {
        matchedB[i] = bufferB[i] * linearGain;
    }

    return matchedB;
}

ABComparisonResult ABComparison::compare(
    const float* originalA,
    const float* processedB,
    size_t numSamples,
    double sampleRate)
{
    ABComparisonResult res;
    if (originalA == nullptr || processedB == nullptr || numSamples == 0)
        return res;

    res.metricsA = AudioMetrics::analyzeBuffer(originalA, numSamples, sampleRate);
    res.metricsB = AudioMetrics::analyzeBuffer(processedB, numSamples, sampleRate);

    // 1. Compensar sonoridad
    float appliedGainDb = 0.0f;
    const auto matchedB = matchLoudness(originalA, processedB, numSamples, sampleRate, &appliedGainDb);
    res.gainCompensationAppliedDb = appliedGainDb;
    res.metricsBMatched = AudioMetrics::analyzeBuffer(matchedB.data(), numSamples, sampleRate);

    // 2. Reducción de rango dinámico lograda
    res.dynamicRangeReductionDb = res.metricsA.crestFactorDb - res.metricsBMatched.crestFactorDb;

    // 3. Diferencia espectral en banda de pisadas vs disparos
    const auto bandsA = FrequencyAnalyzer::analyzeBands(originalA, numSamples, sampleRate);
    const auto bandsB = FrequencyAnalyzer::analyzeBands(matchedB.data(), numSamples, sampleRate);

    res.footstepsBoostDb = bandsB.footstepsDb - bandsA.footstepsDb;
    res.gunCrackAttenuationDb = bandsA.gunCrackDb - bandsB.gunCrackDb;

    // 4. Coeficiente de correlación de Pearson
    double sumAB = 0.0, sumA2 = 0.0, sumB2 = 0.0;
    for (size_t i = 0; i < numSamples; ++i)
    {
        sumAB += static_cast<double>(originalA[i]) * static_cast<double>(matchedB[i]);
        sumA2 += static_cast<double>(originalA[i]) * static_cast<double>(originalA[i]);
        sumB2 += static_cast<double>(matchedB[i]) * static_cast<double>(matchedB[i]);
    }

    if (sumA2 > 1e-12 && sumB2 > 1e-12)
    {
        res.correlationCoefficient = static_cast<float>(sumAB / std::sqrt(sumA2 * sumB2));
    }

    return res;
}

} // namespace dash::bench
