#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace dash::dsp
{

struct TransientSuppressorParams
{
    float gun     = 0.45f; // [0.0 .. 1.0] Ganancia objetivo del pico de disparo (0.45 => -6.94 dB ~ -7 dB)
    float steps   = 0.0f;  // [0.0 .. 1.0] Protección adicional contra micro-transitorios de pasos
    bool  front   = true;  // [0 / 1] Aislar esfera frontal / Canal Central en 5.1 y 7.1
    float restore = 0.0f;  // [0.0 .. 1.0] Compensación suave post-transitorio
    float release = 0.35f; // [0.0 .. 1.0] Relajación ajustable del envelope follower
    bool  bypass  = false;
};

/**
 * Supresor de Transitorios de Disparo ("Gun Crack") en Tiempo Real.
 *
 * Arquitectura fiel a la auditoría:
 *  1. En configuraciones 5.1 / 7.1 aísla prioritariamente el Canal Central (Center In),
 *     dejando intactos los canales surround traseros/laterales y el canal LFE.
 *     En configuración Estéreo (2.0) extrae la componente Mid central (L+R)/2 si Front=1.
 *  2. Divide el espectro con un crossover Linkwitz-Riley de 4º orden (juce::dsp::LinkwitzRileyFilter)
 *     en 280 Hz para separar "Gun Body" (< 280 Hz, graves intactos) de "Gun Crack" (>= 280 Hz).
 *  3. Detecta el pico balístico mediante un seguidor de envolvente diferencial ultra-rápido
 *     (ataque ~0.15 ms, release ajustable) y aplica una reducción dinámica de ~ -7 dB
 *     (20 * log10(0.45) = -6.94 dB) exclusivamente sobre el "crack" agudo sin tocar los graves.
 *  4. Cero asignaciones de memoria en el hilo de audio.
 */
class CenterTransientSuppressor
{
public:
    static constexpr int kMaxChannels = 8;
    static constexpr float kBodyCrackCrossoverHz = 280.0f;

    void prepare(const juce::dsp::ProcessSpec& spec, int centerChannelIdx)
    {
        sampleRate_ = (spec.sampleRate > 8000.0) ? spec.sampleRate : 48000.0;
        centerChannelIndex_ = centerChannelIdx;

        juce::dsp::ProcessSpec filterSpec {};
        filterSpec.sampleRate = sampleRate_;
        filterSpec.maximumBlockSize = spec.maximumBlockSize;
        filterSpec.numChannels = static_cast<juce::uint32>(kMaxChannels);

        bodyCrackCrossover_.prepare(filterSpec);
        bodyCrackCrossover_.setCutoffFrequency(kBodyCrackCrossoverHz);

        reset();
    }

    void reset() noexcept
    {
        bodyCrackCrossover_.reset();
        envFast_.fill(0.0f);
        envSlow_.fill(0.0f);
        smoothedGainDb_.fill(0.0f);
        lastReductionDb_ = 0.0f;
    }

    void updateParameters(const TransientSuppressorParams& params) noexcept
    {
        params_ = params;

        // Ataque ultra-rápido (~0.15 ms) para capturar el frente de onda del disparo
        fastAtkCoeff_ = calcOnePoleCoeff(0.00015f);

        // Release ajustable del detector (20 ms .. 180 ms; por defecto 0.35 -> ~76 ms)
        const float relSec = 0.020f + juce::jlimit(0.0f, 1.0f, params_.release) * 0.160f;
        fastRelCoeff_ = calcOnePoleCoeff(relSec);

        // Envolvente lenta de referencia para discriminar timbre estacionario frente a picos impulsivos
        slowAtkCoeff_ = calcOnePoleCoeff(0.018f);
        slowRelCoeff_ = calcOnePoleCoeff(0.220f);

        // Balística de aplicación de ganancia sobre la banda aguda (Crack)
        gainAtkCoeff_ = calcOnePoleCoeff(0.00020f);
        gainRelCoeff_ = calcOnePoleCoeff(relSec);

        // Mapeo fiel de auditoría: Gun = 0.45 -> 20*log10(0.45) = -6.9357 dB (~ -7.0 dB)
        const float clampedGun = juce::jlimit(0.10f, 1.0f, params_.gun);
        maxAttenuationDb_ = juce::Decibels::gainToDecibels(clampedGun, -24.0f); // Negativo (ej. -6.94 dB)
    }

    void processBlock(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        if (numChannels <= 0 || numSamples <= 0)
            return;

        if (params_.bypass)
        {
            lastReductionDb_ = 0.0f;
            return;
        }

        const int activeCh = juce::jmin(numChannels, kMaxChannels);
        float peakReductionDb = 0.0f;

        // Caso A: Audio Multicanal 5.1 / 7.1 -> Aislar Canal Central (y L/R solo si Front == false)
        if (activeCh >= 6)
        {
            const int cIdx = (centerChannelIndex_ >= 0 && centerChannelIndex_ < activeCh) ? centerChannelIndex_ : 2;

            if (params_.front)
            {
                // Procesamiento quirúrgico exclusivo sobre Center In (índice cIdx)
                float* centerData = buffer.getWritePointer(cIdx);
                for (int i = 0; i < numSamples; ++i)
                {
                    const float grDb = processChannelSample(cIdx, centerData[i]);
                    if (grDb < peakReductionDb)
                        peakReductionDb = grDb;
                }
            }
            else
            {
                // Si Front == 0, protege el trío frontal L (0), R (1) y Center (cIdx)
                const std::array<int, 3> targetChannels { 0, 1, cIdx };
                for (int ch : targetChannels)
                {
                    if (ch >= 0 && ch < activeCh)
                    {
                        float* chData = buffer.getWritePointer(ch);
                        for (int i = 0; i < numSamples; ++i)
                        {
                            const float grDb = processChannelSample(ch, chData[i]);
                            if (grDb < peakReductionDb)
                                peakReductionDb = grDb;
                        }
                    }
                }
            }
        }
        // Caso B: Estéreo (2.0) -> Si Front == 1, aísla el Centro Fantasma Mid = (L+R)*0.5 dejando Side intacto
        else if (activeCh == 2 && params_.front)
        {
            float* leftData  = buffer.getWritePointer(0);
            float* rightData = buffer.getWritePointer(1);

            for (int i = 0; i < numSamples; ++i)
            {
                float mid        = 0.5f * (leftData[i] + rightData[i]);
                const float side = 0.5f * (leftData[i] - rightData[i]);

                const float grDb = processChannelSample(0, mid);
                if (grDb < peakReductionDb)
                    peakReductionDb = grDb;

                leftData[i]  = mid + side;
                rightData[i] = mid - side;
            }
        }
        // Caso C: Mono o Estéreo completo (Front == 0)
        else
        {
            for (int ch = 0; ch < activeCh; ++ch)
            {
                float* chData = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    const float grDb = processChannelSample(ch, chData[i]);
                    if (grDb < peakReductionDb)
                        peakReductionDb = grDb;
                }
            }
        }

        bodyCrackCrossover_.snapToZero();
        lastReductionDb_ = peakReductionDb;
    }

    [[nodiscard]] float getLastReductionDb() const noexcept { return lastReductionDb_; }

private:
    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.00005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate_)));
    }

    /**
     * Procesa una muestra individual separando Gun Body (< 280 Hz) de Gun Crack (>= 280 Hz),
     * aplicando reducción dinámica solo al pico transitorio agudo.
     * Devuelve la reducción de ganancia instantánea en dB (<= 0.0f).
     */
    float processChannelSample(int ch, float& sample) noexcept
    {
        float gunBody  = 0.0f;
        float gunCrack = 0.0f;

        // Separación de fase coherente Linkwitz-Riley: Gun Body (graves) + Gun Crack (agudos)
        bodyCrackCrossover_.processSample(ch, sample, gunBody, gunCrack);

        const float absCrack = std::abs(gunCrack);
        const size_t idx = static_cast<size_t>(ch);

        // Seguidor de envolvente rápido vs lento sobre la banda de transitorios
        const float fCoeff = (absCrack > envFast_[idx]) ? fastAtkCoeff_ : fastRelCoeff_;
        envFast_[idx] = fCoeff * envFast_[idx] + (1.0f - fCoeff) * absCrack;

        const float sCoeff = (absCrack > envSlow_[idx]) ? slowAtkCoeff_ : slowRelCoeff_;
        envSlow_[idx] = sCoeff * envSlow_[idx] + (1.0f - sCoeff) * absCrack;

        // Umbral mínimo de energía (-26 dBFS ~ 0.0501) + discriminación de cresta para proteger pasos (Steps)
        const float minPeakThreshold = 0.050f + params_.steps * 0.085f;
        const float requiredCrestRatio = 1.65f + params_.steps * 1.35f;

        float targetDb = 0.0f;
        if (envFast_[idx] > minPeakThreshold)
        {
            const float crestRatio = envFast_[idx] / (envSlow_[idx] + 1.0e-5f);
            if (crestRatio > requiredCrestRatio)
            {
                // Profundidad proporcional a la severidad del pico balístico, acotada a maxAttenuationDb_ (~ -7 dB)
                const float excessNorm = juce::jlimit(0.0f, 1.0f, (crestRatio - requiredCrestRatio) / 2.5f);
                const float levelNorm  = juce::jlimit(0.0f, 1.0f, (envFast_[idx] - minPeakThreshold) / 0.25f);
                const float transientSeverity = juce::jmax(excessNorm, levelNorm);
                targetDb = maxAttenuationDb_ * transientSeverity;
            }
        }

        // Suavizado balístico de ganancia (ataque instantáneo, release controlado)
        const float gCoeff = (targetDb < smoothedGainDb_[idx]) ? gainAtkCoeff_ : gainRelCoeff_;
        smoothedGainDb_[idx] = gCoeff * smoothedGainDb_[idx] + (1.0f - gCoeff) * targetDb;

        // Compensación opcional post-transitorio (Restore)
        const float restoreBoostDb = params_.restore * 2.5f * (1.0f - juce::jlimit(0.0f, 1.0f, -smoothedGainDb_[idx] / 6.0f));
        const float crackGainLin = juce::Decibels::decibelsToGain(smoothedGainDb_[idx] + restoreBoostDb);

        // Reconstrucción: Gun Body permanece 100% intacto; solo Gun Crack recibe la atenuación dinámica
        sample = gunBody + (gunCrack * crackGainLin);

        return smoothedGainDb_[idx];
    }

    double sampleRate_ = 48000.0;
    int centerChannelIndex_ = 2;
    TransientSuppressorParams params_ {};

    juce::dsp::LinkwitzRileyFilter<float> bodyCrackCrossover_;

    std::array<float, kMaxChannels> envFast_ {};
    std::array<float, kMaxChannels> envSlow_ {};
    std::array<float, kMaxChannels> smoothedGainDb_ {};

    float fastAtkCoeff_ = 0.9f;
    float fastRelCoeff_ = 0.999f;
    float slowAtkCoeff_ = 0.99f;
    float slowRelCoeff_ = 0.9995f;
    float gainAtkCoeff_ = 0.9f;
    float gainRelCoeff_ = 0.999f;

    float maxAttenuationDb_ = -6.93575f; // Gun = 0.45 -> -6.94 dB (~ -7 dB)
    float lastReductionDb_  = 0.0f;
};

} // namespace dash::dsp
