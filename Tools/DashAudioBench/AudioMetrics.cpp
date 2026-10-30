#include "AudioMetrics.h"

#include <algorithm>
#include <cmath>

namespace dash::bench
{

namespace
{
    constexpr float kMinDb = -120.0f;

    float linearToDb(float linear) noexcept
    {
        if (linear <= 1e-6f)
            return kMinDb;
        return 20.0f * std::log10(linear);
    }
} // namespace

float AudioMetrics::calculatePeakDbFs(const float* buffer, size_t numSamples) noexcept
{
    if (buffer == nullptr || numSamples == 0)
        return kMinDb;

    float maxVal = 0.0f;
    for (size_t i = 0; i < numSamples; ++i)
    {
        const float absVal = std::abs(buffer[i]);
        if (absVal > maxVal)
            maxVal = absVal;
    }

    return linearToDb(maxVal);
}

float AudioMetrics::calculateRmsDbFs(const float* buffer, size_t numSamples) noexcept
{
    if (buffer == nullptr || numSamples == 0)
        return kMinDb;

    double sumSq = 0.0;
    for (size_t i = 0; i < numSamples; ++i)
    {
        sumSq += static_cast<double>(buffer[i]) * static_cast<double>(buffer[i]);
    }

    const double rms = std::sqrt(sumSq / static_cast<double>(numSamples));
    return linearToDb(static_cast<float>(rms));
}

float AudioMetrics::calculateLufs(
    const float* const* channelPointers,
    int numChannels,
    size_t numSamples,
    double /*sampleRate*/) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples == 0)
        return kMinDb;

    // Cálculo ponderado de media cuadrática multicanal simplificado ITU-R BS.1770-4
    double weightedMeanSquare = 0.0;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* buf = channelPointers[ch];
        if (buf == nullptr)
            continue;

        // Peso BS.1770: LFE (ch == 3 en 5.1/7.1) = 0.0; Surround = 1.41; Frontales = 1.0
        double channelWeight = 1.0;
        if (ch == 3 && numChannels >= 6) // LFE canal
            channelWeight = 0.0;
        else if (ch >= 4) // Surround
            channelWeight = 1.41;

        if (channelWeight > 0.0)
        {
            double chSumSq = 0.0;
            for (size_t i = 0; i < numSamples; ++i)
            {
                chSumSq += static_cast<double>(buf[i]) * static_cast<double>(buf[i]);
            }
            weightedMeanSquare += channelWeight * (chSumSq / static_cast<double>(numSamples));
        }
    }

    if (weightedMeanSquare <= 1e-12)
        return kMinDb;

    return static_cast<float>(-0.691 + 10.0 * std::log10(weightedMeanSquare));
}

size_t AudioMetrics::countClippingSamples(const float* buffer, size_t numSamples, float threshold) noexcept
{
    if (buffer == nullptr || numSamples == 0)
        return 0;

    size_t count = 0;
    for (size_t i = 0; i < numSamples; ++i)
    {
        if (std::abs(buffer[i]) >= threshold)
            ++count;
    }
    return count;
}

float AudioMetrics::calculateTHD(
    const float* buffer,
    size_t numSamples,
    double sampleRate,
    float testFreqHz) noexcept
{
    if (buffer == nullptr || numSamples < 128 || sampleRate <= 0.0 || testFreqHz <= 0.0f)
        return 0.0f;

    // Implementación Goertzel/DFT para fundamental vs armónicos
    const double w0 = 2.0 * 3.14159265358979323846 * static_cast<double>(testFreqHz) / sampleRate;

    double fundReal = 0.0;
    double fundImag = 0.0;
    for (size_t i = 0; i < numSamples; ++i)
    {
        fundReal += buffer[i] * std::cos(w0 * static_cast<double>(i));
        fundImag -= buffer[i] * std::sin(w0 * static_cast<double>(i));
    }
    const double fundamentalMag = std::sqrt(fundReal * fundReal + fundImag * fundImag);

    if (fundamentalMag <= 1e-6)
        return 0.0f;

    // Armónicos 2do al 5to
    double harmonicsSumSq = 0.0;
    for (int h = 2; h <= 5; ++h)
    {
        const double wh = w0 * h;
        if (wh >= 3.14159265358979323846) // Nyquist
            break;

        double hReal = 0.0;
        double hImag = 0.0;
        for (size_t i = 0; i < numSamples; ++i)
        {
            hReal += buffer[i] * std::cos(wh * static_cast<double>(i));
            hImag -= buffer[i] * std::sin(wh * static_cast<double>(i));
        }
        const double hMag = std::sqrt(hReal * hReal + hImag * hImag);
        harmonicsSumSq += hMag * hMag;
    }

    const double thd = std::sqrt(harmonicsSumSq) / fundamentalMag;
    return static_cast<float>(thd * 100.0);
}

QualityMetrics AudioMetrics::analyzeBuffer(const float* buffer, size_t numSamples, double sampleRate) noexcept
{
    QualityMetrics m;
    m.peakDbFs = calculatePeakDbFs(buffer, numSamples);
    m.rmsDbFs  = calculateRmsDbFs(buffer, numSamples);
    m.crestFactorDb = m.peakDbFs - m.rmsDbFs;
    m.clippedSamples = countClippingSamples(buffer, numSamples);
    m.isClipped = (m.clippedSamples > 0);
    m.thdPercent = calculateTHD(buffer, numSamples, sampleRate, 1000.0f);
    m.snrDb = std::abs(m.peakDbFs - kMinDb);
    return m;
}

} // namespace dash::bench
