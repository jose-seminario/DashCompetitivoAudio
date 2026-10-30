#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstddef>

namespace dash::dsp
{

/**
 * Filtro de ponderación perceptual K (ITU-R BS.1770-4) por canal.
 *
 * Consta de dos etapas biquad en cascada especificadas por el estándar:
 *  - Etapa 1 (Pre-filter acústico de cabeza esférica): High-Shelf +3.99984 dB @ 1681.97 Hz, Q = 0.707175
 *  - Etapa 2 (Filtro RLB - Revised Low-frequency B-weighting): High-Pass @ 38.13547 Hz, Q = 0.500327
 *
 * Libre de asignaciones dinámicas durante processSample().
 */
class KWeightingFilter
{
public:
    static constexpr size_t kMaxChannels = 8; // Soporta desde Mono/Estéreo hasta 7.1 Surround

    void prepare(double sampleRate, int numChannels)
    {
        const double safeSr = (sampleRate > 8000.0) ? sampleRate : 48000.0;

        auto stage1Coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            safeSr,
            1681.97445f,
            0.707175237f,
            juce::Decibels::decibelsToGain(3.99984385f));

        auto stage2Coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
            safeSr,
            38.1354709f,
            0.500327037f);

        activeChannels_ = static_cast<size_t>(juce::jlimit(1, static_cast<int>(kMaxChannels), numChannels));

        juce::dsp::ProcessSpec monoSpec {};
        monoSpec.sampleRate = safeSr;
        monoSpec.maximumBlockSize = 512;
        monoSpec.numChannels = 1;

        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            channels_[ch].preFilter.prepare(monoSpec);
            channels_[ch].rlbFilter.prepare(monoSpec);
            channels_[ch].preFilter.coefficients = stage1Coeffs;
            channels_[ch].rlbFilter.coefficients = stage2Coeffs;
            channels_[ch].preFilter.reset();
            channels_[ch].rlbFilter.reset();
        }
    }

    void reset() noexcept
    {
        for (auto& ch : channels_)
        {
            ch.preFilter.reset();
            ch.rlbFilter.reset();
        }
    }

    [[nodiscard]] float processSample(size_t channelIndex, float inputSample) noexcept
    {
        if (channelIndex >= activeChannels_)
            return inputSample;

        auto& ch = channels_[channelIndex];
        const float afterStage1 = ch.preFilter.processSample(inputSample);
        return ch.rlbFilter.processSample(afterStage1);
    }

    /**
     * Ponderación espacial por canal según ITU-R BS.1770-4:
     *  - L, R, C (0, 1, 2): 1.0 (0 dB)
     *  - LFE (3 en configuraciones >= 6 canales): 0.0 (excluido de la medición LUFS)
     *  - Surrounds Ls, Rs, Lrs, Rrs (4..7): 1.41 (~ +1.5 dB)
     */
    [[nodiscard]] static float getChannelWeight(size_t channelIndex, size_t totalChannels) noexcept
    {
        if (totalChannels >= 6)
        {
            if (channelIndex == 3)
                return 0.0f; // Canal LFE excluido según BS.1770-4
            if (channelIndex >= 4)
                return 1.41f;
        }
        return 1.0f;
    }

private:
    struct ChannelStagePair
    {
        juce::dsp::IIR::Filter<float> preFilter;
        juce::dsp::IIR::Filter<float> rlbFilter;
    };

    std::array<ChannelStagePair, kMaxChannels> channels_;
    size_t activeChannels_ = 2;
};

} // namespace dash::dsp
