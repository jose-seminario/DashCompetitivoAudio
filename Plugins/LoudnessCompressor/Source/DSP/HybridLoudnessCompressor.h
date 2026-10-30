#pragma once

#include "KWeightingFilter.h"

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace dash::dsp
{

struct CompressorParameters
{
    float amount       = 0.75f;     // [0.0 .. 1.0] Intensidad global híbrida (Upward + Downward)
    float targetLufs   = -18.0f;    // [-28.0 .. +8.0] Objetivo ITU-R BS.1770-4 (-18 LUFS por defecto)
    float releaseNorm  = 0.50f;     // [0.0 .. 1.0] Relajación de ganancia del compresor
    float detRelNorm   = 0.583333f; // [0.0 .. 1.0] Relajación del detector RMS (~400 ms BS.1770 Momentary)
    float limRelNorm   = 0.612903f; // [0.0 .. 1.0] Relajación del limitador de picos de salida
    float scLowNorm    = 0.0f;      // [0.0 .. 1.0] Frecuencia de corte inferior del sidechain
    float scHighNorm   = 1.0f;      // [0.0 .. 1.0] Frecuencia de corte superior del sidechain
    float maxLiftDb    = 9.0f;      // [0.0 .. 18.0] Ganancia ascendente máxima (Upward Lift para pasos)
    float gateFloorDb  = -62.0f;    // [-80.0 .. -35.0] Umbral inferior anti-ruido de fondo
    bool  bypass       = false;
};

/**
 * Motor DSP en tiempo real de Compresión Dinámica Híbrida (Upward + Downward)
 * gobernado por detección RMS ponderada K según ITU-R BS.1770-4.
 *
 * Propiedades de tiempo real:
 *  - Cero asignaciones de memoria (lock-free / allocation-free) en processBlock().
 *  - Filtros TPT (Topology-Preserving Transform) de estado variable para el sidechain.
 *  - Limitador de techo de seguridad integrado a -0.5 dBFS.
 */
class HybridLoudnessCompressor
{
public:
    static constexpr size_t kMaxChannels = KWeightingFilter::kMaxChannels;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate_ = (spec.sampleRate > 8000.0) ? spec.sampleRate : 48000.0;
        const int clampedCh = juce::jlimit(1, static_cast<int>(kMaxChannels), static_cast<int>(spec.numChannels));

        kWeighting_.prepare(sampleRate_, clampedCh);

        juce::dsp::ProcessSpec monoSpec {};
        monoSpec.sampleRate = sampleRate_;
        monoSpec.maximumBlockSize = spec.maximumBlockSize;
        monoSpec.numChannels = 1;

        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            scHighPass_[ch].prepare(monoSpec);
            scHighPass_[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
            scHighPass_[ch].setCutoffFrequency(20.0f);

            scLowPass_[ch].prepare(monoSpec);
            scLowPass_[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            scLowPass_[ch].setCutoffFrequency(20000.0f);
        }

        reset();
    }

    void reset() noexcept
    {
        kWeighting_.reset();
        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            scHighPass_[ch].reset();
            scLowPass_[ch].reset();
        }

        meanSquareEnv_  = 1.0e-8f;
        smoothedGainDb_ = 0.0f;
        limiterGainLin_ = 1.0f;
        lastLufs_       = -70.0f;
        lastGainDb_     = 0.0f;
    }

    void updateParameters(const CompressorParameters& params) noexcept
    {
        params_ = params;

        // Mapeo logarítmico de frecuencias del detector Sidechain [20 Hz .. 20 kHz]
        const float hpHz = mapNormToFreqHz(juce::jlimit(0.0f, 0.95f, params_.scLowNorm));
        const float lpHz = juce::jmax(hpHz + 80.0f, mapNormToFreqHz(juce::jlimit(0.05f, 1.0f, params_.scHighNorm)));

        const float nyquistGuard = static_cast<float>(sampleRate_ * 0.48);
        const float safeHp = juce::jlimit(20.0f, nyquistGuard, hpHz);
        const float safeLp = juce::jlimit(80.0f, nyquistGuard, lpHz);

        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            scHighPass_[ch].setCutoffFrequency(safeHp);
            scLowPass_[ch].setCutoffFrequency(safeLp);
        }

        // Constantes de tiempo del detector RMS BS.1770 (Ataque 15 ms, Relajación 40..700 ms; 0.583333 -> ~400 ms)
        detAtkCoeff_ = calcOnePoleCoeff(0.015f);
        const float detRelSec = 0.040f + juce::jlimit(0.0f, 1.0f, params_.detRelNorm) * 0.620f;
        detRelCoeff_ = calcOnePoleCoeff(detRelSec);

        // Constantes de tiempo de la computadora de ganancia híbrida
        gainAtkCoeff_ = calcOnePoleCoeff(0.006f);
        const float compRelSec = 0.025f + juce::jlimit(0.0f, 1.0f, params_.releaseNorm) * 0.450f;
        gainRelCoeff_ = calcOnePoleCoeff(compRelSec);

        // Constante de tiempo del limitador final de picos (15..250 ms)
        const float limRelSec = 0.015f + juce::jlimit(0.0f, 1.0f, params_.limRelNorm) * 0.235f;
        limRelCoeff_ = calcOnePoleCoeff(limRelSec);
    }

    void processBlock(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        if (numChannels <= 0 || numSamples <= 0)
            return;

        if (params_.bypass)
        {
            lastGainDb_ = 0.0f;
            return;
        }

        const size_t activeCh = static_cast<size_t>(juce::jmin(numChannels, static_cast<int>(kMaxChannels)));
        const bool useHp = (params_.scLowNorm > 0.001f);
        const bool useLp = (params_.scHighNorm < 0.999f);

        std::array<float*, kMaxChannels> channelPtrs {};
        for (size_t ch = 0; ch < activeCh; ++ch)
            channelPtrs[ch] = buffer.getWritePointer(static_cast<int>(ch));

        float blockLufs   = lastLufs_;
        float blockGainDb = lastGainDb_;

        constexpr float kLimiterCeilingLin = 0.94406088f; // -0.5 dBFS

        for (int i = 0; i < numSamples; ++i)
        {
            // 1. Medición de energía instantánea ponderada K (ITU-R BS.1770-4) + filtro Sidechain
            float weightedSquareSum = 0.0f;

            for (size_t ch = 0; ch < activeCh; ++ch)
            {
                const float x = channelPtrs[ch][i];
                const float weight = KWeightingFilter::getChannelWeight(ch, activeCh);

                if (weight <= 0.0f)
                    continue;

                float sc = kWeighting_.processSample(ch, x);
                if (useHp)
                    sc = scHighPass_[ch].processSample(0, sc);
                if (useLp)
                    sc = scLowPass_[ch].processSample(0, sc);

                weightedSquareSum += weight * (sc * sc);
            }

            // 2. Detector RMS (Media cuadrática con balística asimétrica)
            const float detCoeff = (weightedSquareSum > meanSquareEnv_) ? detAtkCoeff_ : detRelCoeff_;
            meanSquareEnv_ = detCoeff * meanSquareEnv_ + (1.0f - detCoeff) * weightedSquareSum;

            // Conversión oficial ITU-R BS.1770-4: L_K = -0.691 + 10 * log10(sum(G_i * z_i))
            const float currentLufs = -0.691f + 10.0f * std::log10(juce::jmax(meanSquareEnv_, 1.0e-12f));
            blockLufs = currentLufs;

            // 3. Computadora de Ganancia Híbrida (Upward + Downward alrededor de Target = -18 LUFS)
            const float targetGainDb = computeHybridTargetGainDb(currentLufs);

            // Nota: cuando reducimos ganancia (targetGainDb < smoothedGainDb_) actuamos con ataque rápido;
            // cuando recuperamos o elevamos ganancia ascendente actuamos con release controlado.
            const float gainCoeff = (targetGainDb < smoothedGainDb_) ? gainAtkCoeff_ : gainRelCoeff_;
            smoothedGainDb_ = gainCoeff * smoothedGainDb_ + (1.0f - gainCoeff) * targetGainDb;
            blockGainDb = smoothedGainDb_;

            const float compGainLin = juce::Decibels::decibelsToGain(smoothedGainDb_);

            // 4. Aplicación de ganancia + Limitador de seguridad de muestra exacta
            float peakPostComp = 0.0f;
            for (size_t ch = 0; ch < activeCh; ++ch)
            {
                const float amplified = std::abs(channelPtrs[ch][i] * compGainLin);
                if (amplified > peakPostComp)
                    peakPostComp = amplified;
            }

            float desiredLimGain = 1.0f;
            if (peakPostComp > kLimiterCeilingLin)
                desiredLimGain = kLimiterCeilingLin / peakPostComp;

            if (desiredLimGain < limiterGainLin_)
                limiterGainLin_ = desiredLimGain; // Ataque instantáneo para contener transitorios
            else
                limiterGainLin_ = limRelCoeff_ * limiterGainLin_ + (1.0f - limRelCoeff_) * 1.0f;

            const float totalGainLin = compGainLin * limiterGainLin_;

            for (size_t ch = 0; ch < activeCh; ++ch)
                channelPtrs[ch][i] *= totalGainLin;
        }

        lastLufs_   = blockLufs;
        lastGainDb_ = blockGainDb + juce::Decibels::gainToDecibels(limiterGainLin_, -60.0f);
    }

    [[nodiscard]] float getCurrentLufs() const noexcept   { return lastLufs_; }
    [[nodiscard]] float getCurrentGainDb() const noexcept { return lastGainDb_; }

private:
    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate_)));
    }

    [[nodiscard]] static float mapNormToFreqHz(float norm) noexcept
    {
        // Mapeo exponencial de 20 Hz a 20.000 Hz (3 décadas)
        return 20.0f * std::pow(1000.0f, norm);
    }

    [[nodiscard]] float computeHybridTargetGainDb(float currentLufs) const noexcept
    {
        const float amount = juce::jlimit(0.0f, 1.0f, params_.amount);
        if (amount <= 0.0001f)
            return 0.0f;

        const float deltaLufs = currentLufs - params_.targetLufs;

        if (deltaLufs > 0.0f)
        {
            // Rama Downward Compression: atenúa explosiones/disparos que superan -18 LUFS (Ratio máx ~ 4:1)
            constexpr float kMaxDownwardSlope = 0.75f; // 1 - 1/4
            return -deltaLufs * (kMaxDownwardSlope * amount);
        }

        // Rama Upward Compression: eleva micro-detalles (pasos, recargas) por debajo de -18 LUFS
        // respetando una puerta suave (Gate) para no amplificar el piso de ruido en silencio.
        if (currentLufs <= params_.gateFloorDb)
            return 0.0f;

        constexpr float kGateKneeWidthDb = 10.0f;
        float gateWeight = 1.0f;
        if (currentLufs < params_.gateFloorDb + kGateKneeWidthDb)
        {
            gateWeight = (currentLufs - params_.gateFloorDb) / kGateKneeWidthDb;
            gateWeight = gateWeight * gateWeight * (3.0f - 2.0f * gateWeight); // Smoothstep C1
        }

        const float deficitDb = -deltaLufs; // Positivo cuando currentLufs < targetLufs
        constexpr float kUpwardRatioSlope = 0.55f;
        const float rawBoostDb = deficitDb * kUpwardRatioSlope * amount;
        const float clampedBoostDb = juce::jmin(rawBoostDb, params_.maxLiftDb * amount);

        return clampedBoostDb * gateWeight;
    }

    double sampleRate_ = 48000.0;
    CompressorParameters params_ {};

    KWeightingFilter kWeighting_;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kMaxChannels> scHighPass_;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kMaxChannels> scLowPass_;

    float detAtkCoeff_  = 0.99f;
    float detRelCoeff_  = 0.999f;
    float gainAtkCoeff_ = 0.99f;
    float gainRelCoeff_ = 0.999f;
    float limRelCoeff_  = 0.999f;

    float meanSquareEnv_  = 1.0e-8f;
    float smoothedGainDb_ = 0.0f;
    float limiterGainLin_ = 1.0f;

    float lastLufs_   = -70.0f;
    float lastGainDb_ = 0.0f;
};

} // namespace dash::dsp
