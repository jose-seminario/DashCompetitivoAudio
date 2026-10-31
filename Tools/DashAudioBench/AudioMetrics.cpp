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

    struct KWeightingBiquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;

        void reset() noexcept
        {
            x1 = x2 = y1 = y2 = 0.0;
        }

        [[nodiscard]] double process(double in) noexcept
        {
            const double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = in;
            y2 = y1;
            y1 = out;
            return out;
        }
    };

    void getKWeightingCoefficients(double sampleRate, KWeightingBiquad& stage1, KWeightingBiquad& stage2) noexcept
    {
        constexpr double kPi = 3.14159265358979323846;
        const double sr = (sampleRate > 8000.0) ? sampleRate : 48000.0;

        // Stage 1: High shelf a 1681.97445 Hz, Q = 0.707175237, Ganancia = +3.99984385 dB
        {
            const double f0 = 1681.97445;
            const double gainDb = 3.99984385;
            const double q = 0.707175237;

            const double w0 = 2.0 * kPi * f0 / sr;
            const double cosW = std::cos(w0);
            const double sinW = std::sin(w0);
            const double alpha = sinW / (2.0 * q);
            const double A = std::pow(10.0, gainDb / 40.0);
            const double sqrtA = std::sqrt(A);

            const double b0 = A * ((A + 1.0) + (A - 1.0) * cosW + 2.0 * sqrtA * alpha);
            const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW);
            const double b2 = A * ((A + 1.0) + (A - 1.0) * cosW - 2.0 * sqrtA * alpha);
            const double a0 = (A + 1.0) - (A - 1.0) * cosW + 2.0 * sqrtA * alpha;
            const double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW);
            const double a2 = (A + 1.0) - (A - 1.0) * cosW - 2.0 * sqrtA * alpha;

            const double invA0 = 1.0 / a0;
            stage1.b0 = b0 * invA0;
            stage1.b1 = b1 * invA0;
            stage1.b2 = b2 * invA0;
            stage1.a1 = a1 * invA0;
            stage1.a2 = a2 * invA0;
            stage1.reset();
        }

        // Stage 2: Filtro RLB pasa-altos a 38.1354709 Hz, Q = 0.500327037
        {
            const double f0 = 38.1354709;
            const double q = 0.500327037;

            const double w0 = 2.0 * kPi * f0 / sr;
            const double cosW = std::cos(w0);
            const double sinW = std::sin(w0);
            const double alpha = sinW / (2.0 * q);

            const double b0 = (1.0 + cosW) * 0.5;
            const double b1 = -(1.0 + cosW);
            const double b2 = (1.0 + cosW) * 0.5;
            const double a0 = 1.0 + alpha;
            const double a1 = -2.0 * cosW;
            const double a2 = 1.0 - alpha;

            const double invA0 = 1.0 / a0;
            stage2.b0 = b0 * invA0;
            stage2.b1 = b1 * invA0;
            stage2.b2 = b2 * invA0;
            stage2.a1 = a1 * invA0;
            stage2.a2 = a2 * invA0;
            stage2.reset();
        }
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
    double sampleRate) noexcept
{
    if (channelPointers == nullptr || numChannels <= 0 || numSamples == 0)
        return kMinDb;

    // Cálculo ponderado K-Weighting multicanal (ITU-R BS.1770-4)
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
            KWeightingBiquad stage1, stage2;
            getKWeightingCoefficients(sampleRate, stage1, stage2);

            double chSumSq = 0.0;
            for (size_t i = 0; i < numSamples; ++i)
            {
                const double stage1Out = stage1.process(static_cast<double>(buf[i]));
                const double kWeighted = stage2.process(stage1Out);
                chSumSq += kWeighted * kWeighted;
            }
            weightedMeanSquare += channelWeight * (chSumSq / static_cast<double>(numSamples));
        }
    }

    if (weightedMeanSquare <= 1e-12)
        return -70.0f;

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

    // Cálculo real de SNR (RMS de la señal fundamental vs RMS del residuo/ruido)
    if (buffer != nullptr && numSamples >= 128 && sampleRate > 0.0)
    {
        constexpr double testFreqHz = 1000.0;
        const double w0 = 2.0 * 3.14159265358979323846 * testFreqHz / sampleRate;

        double fundReal = 0.0;
        double fundImag = 0.0;
        for (size_t i = 0; i < numSamples; ++i)
        {
            fundReal += buffer[i] * std::cos(w0 * static_cast<double>(i));
            fundImag -= buffer[i] * std::sin(w0 * static_cast<double>(i));
        }

        const double A = (2.0 / static_cast<double>(numSamples)) * fundReal;
        const double B = -(2.0 / static_cast<double>(numSamples)) * fundImag;
        const double signalRms = std::sqrt((A * A + B * B) * 0.5);

        if (signalRms > 1.0e-5)
        {
            double noiseSumSq = 0.0;
            for (size_t i = 0; i < numSamples; ++i)
            {
                const double reconstructed = A * std::cos(w0 * static_cast<double>(i)) - B * std::sin(w0 * static_cast<double>(i));
                const double residual = static_cast<double>(buffer[i]) - reconstructed;
                noiseSumSq += residual * residual;
            }
            const double noiseRms = std::sqrt(noiseSumSq / static_cast<double>(numSamples));
            if (noiseRms <= 1.0e-6)
                m.snrDb = 120.0f;
            else
                m.snrDb = static_cast<float>(20.0 * std::log10(signalRms / noiseRms));
        }
        else
        {
            m.snrDb = 0.0f;
        }
    }
    else
    {
        m.snrDb = 0.0f;
    }

    return m;
}

} // namespace dash::bench
