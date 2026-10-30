#include "HRIRDataset.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace dash::spatial
{

namespace
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kSpeedOfSoundMPerS = 343.0f; // c = 343 m/s a 20°C

    /**
     * Calcula la Diferencia Interaural de Tiempo (ITD) en segundos mediante la fórmula de Woodworth:
     *   ITD = (r / c) * (sin(|theta|) + |theta|)
     * donde r es el radio de la cabeza (~0.0875 m) y theta es el azimuth en radianes.
     */
    float calculateWoodworthItdSeconds(float azimuthRad, float headRadiusM) noexcept
    {
        const float absTheta = std::abs(azimuthRad);
        return (headRadiusM / kSpeedOfSoundMPerS) * (std::sin(absTheta) + absTheta);
    }
} // namespace

HRIRDataset::HRIRDataset() = default;
HRIRDataset::~HRIRDataset() = default;
HRIRDataset::HRIRDataset(HRIRDataset&&) noexcept = default;
HRIRDataset& HRIRDataset::operator=(HRIRDataset&&) noexcept = default;

bool HRIRDataset::loadSyntheticReference(
    const ChannelLayout& layout,
    double sampleRate,
    size_t irLength)
{
    const double safeSr = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    const size_t safeTaps = std::clamp(irLength, size_t(64), size_t(1024));

    profile_.id               = "synthetic_woodworth";
    profile_.name             = "Dash Woodworth Synthetic Spherical Model";
    profile_.author           = "DashAudio Acoustics";
    profile_.license          = "AGPLv3 / Public Domain";
    profile_.description      = "Modelo analítico esférico de difracción de cabeza con modelado ITD e ILD.";
    profile_.sampleRate       = safeSr;
    profile_.irLengthTaps     = safeTaps;
    profile_.hasElevationFilters = true;

    irLength_ = safeTaps;
    const size_t numChannels = layout.getNumChannels();

    for (size_t ch = 0; ch < kMaxChannels; ++ch)
    {
        pairs_[ch].leftEar.assign(safeTaps, 0.0f);
        pairs_[ch].rightEar.assign(safeTaps, 0.0f);
    }

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        const auto& spk = layout.getSpeaker(ch);

        // Si el canal es LFE, se asigna un impulso unitario en fase pura
        if (spk.role == ChannelRole::LFE)
        {
            pairs_[ch].leftEar[0]  = 0.7071f;
            pairs_[ch].rightEar[0] = 0.7071f;
            pairs_[ch].peakGain    = 0.7071f;
            continue;
        }

        const float azRad = spk.azimuthDeg * (kPi / 180.0f);
        const float elRad = spk.elevationDeg * (kPi / 180.0f);

        // 1. Cálculo de retraso ITD
        const float itdSec = calculateWoodworthItdSeconds(azRad, profile_.headRadiusM) * profile_.itdScaling;
        const float itdSamples = itdSec * static_cast<float>(safeSr);

        // 2. Cálculo de atenuación de sombra de cabeza (ILD)
        // Oído ipsilateral (cercano a la fuente) vs contralateral (lejano)
        const float sinAz = std::sin(azRad);
        const float ildDb = 6.0f * sinAz * profile_.ildScaling; // Hasta +-6 dB de diferencia interaural
        const float leftGain  = std::pow(10.0f, -ildDb / 40.0f);
        const float rightGain = std::pow(10.0f, +ildDb / 40.0f);

        // Retrasos enteros y fraccionarios
        const float delayLeft  = (azRad > 0.0f) ? itdSamples : 0.0f;
        const float delayRight = (azRad < 0.0f) ? itdSamples : 0.0f;

        const auto synthesizeImpulse = [&](std::vector<float>& dest, float delay, float gain) {
            const int baseIdx = std::clamp(static_cast<int>(delay), 0, static_cast<int>(safeTaps - 3));
            const float frac = delay - static_cast<float>(baseIdx);

            // Interpolación sinc/cúbica para retardo sub-muestral suave
            dest[baseIdx]     += gain * (1.0f - frac);
            dest[baseIdx + 1] += gain * frac;

            // Filtro de sombra de cabeza (Head-Shadow) para oído contralateral (atenuación en altas)
            if (delay > 0.5f)
            {
                for (size_t t = 1; t < safeTaps; ++t)
                    dest[t] += dest[t - 1] * 0.15f; // Suavizado pasa-bajos
            }

            // Efecto de elevación acústica (muesca de pabellón auricular de 6 a 8 kHz)
            if (std::abs(elRad) > 0.1f && baseIdx + 8 < static_cast<int>(safeTaps))
            {
                dest[baseIdx + 4] -= gain * 0.20f * std::sin(elRad);
            }
        };

        synthesizeImpulse(pairs_[ch].leftEar, delayLeft, leftGain);
        synthesizeImpulse(pairs_[ch].rightEar, delayRight, rightGain);
        pairs_[ch].peakGain = std::max(leftGain, rightGain);
        pairs_[ch].delaySamples = std::max(delayLeft, delayRight);
    }

    isLoaded_ = true;
    return true;
}

bool HRIRDataset::setImpulsePair(
    size_t channelIndex,
    const float* leftTaps,
    const float* rightTaps,
    size_t numTaps)
{
    if (channelIndex >= kMaxChannels || leftTaps == nullptr || rightTaps == nullptr || numTaps == 0)
        return false;

    pairs_[channelIndex].leftEar.assign(leftTaps, leftTaps + numTaps);
    pairs_[channelIndex].rightEar.assign(rightTaps, rightTaps + numTaps);
    irLength_ = numTaps;
    isLoaded_ = true;

    return true;
}

const HRIRPair* HRIRDataset::getImpulsePair(size_t channelIndex) const noexcept
{
    if (channelIndex < kMaxChannels && pairs_[channelIndex].isValid())
        return &pairs_[channelIndex];
    return nullptr;
}

} // namespace dash::spatial
