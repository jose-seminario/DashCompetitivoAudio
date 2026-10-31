#include "FrequencyAnalyzer.h"

#include <cmath>

namespace dash::bench
{

namespace
{
    float calculateBandRms(
        const float* buffer,
        size_t numSamples,
        double sampleRate,
        float centerFreq,
        float q) noexcept
    {
        // Filtro IIR simple pasa-banda biquad para aislar la región
        const double w0 = 2.0 * 3.14159265358979323846 * centerFreq / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * q);

        const double b0 = alpha;
        const double b1 = 0.0;
        const double b2 = -alpha;
        const double a0 = 1.0 + alpha;
        const double a1 = -2.0 * std::cos(w0);
        const double a2 = 1.0 - alpha;

        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        double sumSq = 0.0;

        for (size_t i = 0; i < numSamples; ++i)
        {
            const double in = buffer[i];
            const double out = (b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2) / a0;

            x2 = x1; x1 = in;
            y2 = y1; y1 = out;

            sumSq += out * out;
        }

        const double rms = std::sqrt(sumSq / static_cast<double>(numSamples));
        if (rms <= 1e-6)
            return -120.0f;
        return static_cast<float>(20.0 * std::log10(rms));
    }
} // namespace

SpectralBandsEnergy FrequencyAnalyzer::analyzeBands(const float* buffer, size_t numSamples, double sampleRate) noexcept
{
    SpectralBandsEnergy e;
    if (buffer == nullptr || numSamples == 0 || sampleRate <= 0.0)
        return e;

    e.subBassDb   = calculateBandRms(buffer, numSamples, sampleRate, 50.0f, 1.0f);
    e.bassDb      = calculateBandRms(buffer, numSamples, sampleRate, 150.0f, 1.2f);
    e.lowMidsDb   = calculateBandRms(buffer, numSamples, sampleRate, 500.0f, 1.2f);
    e.midsDb      = calculateBandRms(buffer, numSamples, sampleRate, 1500.0f, 1.2f);
    e.footstepsDb = calculateBandRms(buffer, numSamples, sampleRate, 3800.0f, 1.5f);
    e.gunCrackDb  = calculateBandRms(buffer, numSamples, sampleRate, 3500.0f, 1.0f);
    e.highsDb     = calculateBandRms(buffer, numSamples, sampleRate, 10000.0f, 1.0f);

    return e;
}

std::vector<float> FrequencyAnalyzer::generateSineSweep(double sampleRate, size_t numSamples, float startFreq, float endFreq)
{
    std::vector<float> sweep(numSamples, 0.0f);
    if (numSamples == 0 || sampleRate <= 0.0)
        return sweep;

    const double duration = static_cast<double>(numSamples) / sampleRate;
    const double k = (duration * 2.0 * 3.14159265358979323846 * startFreq) / std::log(endFreq / startFreq);
    const double l = std::log(endFreq / startFreq) / duration;

    for (size_t i = 0; i < numSamples; ++i)
    {
        const double t = static_cast<double>(i) / sampleRate;
        const double phase = k * (std::exp(t * l) - 1.0);
        sweep[i] = static_cast<float>(std::sin(phase) * 0.5); // -6 dBFS
    }

    return sweep;
}

} // namespace dash::bench
