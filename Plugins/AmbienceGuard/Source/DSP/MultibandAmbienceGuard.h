#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace dash::dsp
{

struct AmbienceGuardParams
{
    float ambGuar = 1.0f;      // [0.0 .. 1.0] Atenuación selectiva de ruido ambiental estacionario
    bool  distant = true;      // [0 / 1] Protege pistas dinámicas lejanas (pasos/voces) frente al piso ambiental
    float target  = 0.388889f; // [0.0 .. 1.0] Umbral auditado (-14 LUFS / -24 dBFS RMS en escala normalizada)
    float release = 0.0f;      // [0.0 .. 1.0] Relajación de ganancia del compresor
    float detRel  = 0.458333f; // [0.0 .. 1.0] Relajación del detector RMS (~295 ms)
    float scLow   = 0.2269f;   // [0.0 .. 1.0] 0.2269 => 350.0 Hz exactos (High-Pass IIR Sidechain)
    float scHigh  = 0.5293f;   // [0.0 .. 1.0] 0.5293 => 2500.0 Hz (2.5 kHz) exactos (Low-Pass IIR Sidechain)
    bool  bandMod = true;      // [0 / 1] Procesamiento Multibanda de 3 bandas (1 = activo por defecto)
    bool  bypass  = false;
};

/**
 * Compresor Multibanda y Protector de Frecuencias Ambientales (AmbienceGuard).
 *
 * Fidelidad matemática respecto a la auditoría:
 *  1. Detector Sidechain restringido estrictamente mediante filtros biquad juce::dsp::IIR
 *     (High-Pass @ SCLow=0.2269 -> 350 Hz y Low-Pass @ SCHigh=0.5293 -> 2.5 kHz),
 *     actualizados sin asignaciones dinámicas usando juce::dsp::IIR::ArrayCoefficients.
 *     Esto impide que graves de explosiones o motores disparen el detector táctico.
 *  2. División Multibanda coherente en fase de 3 bandas (BandMod = 1) mediante crossovers
 *     Linkwitz-Riley de 4º orden (< 350 Hz, 350 Hz - 2.5 kHz, > 2.5 kHz).
 *  3. Estimador dual de estacionariedad (corto plazo vs suelo de ruido lento) que aplica
 *     Downward Compression suave sobre ruido estacionario (AmbGuar = 1) por encima del
 *     umbral Target (0.388889), preservando pasos y voces articuladas (Distant = 1).
 */
class MultibandAmbienceGuard
{
public:
    static constexpr size_t kMaxChannels = 8;

    /**
     * Curva logarítmica monótona calibrada sobre los puntos exactos de auditoría:
     *  - 0.0000 ->    20.0 Hz
     *  - 0.2269 ->   350.0 Hz (SCLow por defecto)
     *  - 0.5293 ->  2500.0 Hz (SCHigh por defecto = 2.5 kHz)
     *  - 1.0000 -> 20000.0 Hz
     */
    [[nodiscard]] static float mapScNormToHz(float norm) noexcept
    {
        const float x = juce::jlimit(0.0f, 1.0f, norm);

        if (x <= 0.2269f)
        {
            const float t = x / 0.2269f;
            return 20.0f * std::pow(350.0f / 20.0f, t);
        }

        if (x <= 0.5293f)
        {
            const float t = (x - 0.2269f) / (0.5293f - 0.2269f);
            return 350.0f * std::pow(2500.0f / 350.0f, t);
        }

        const float t = (x - 0.5293f) / (1.0f - 0.5293f);
        return 2500.0f * std::pow(20000.0f / 2500.0f, t);
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate_ = (spec.sampleRate > 8000.0) ? spec.sampleRate : 48000.0;

        // Pre-asignar una sola vez los coeficientes IIR en prepareToPlay() (fuera del hilo de audio)
        sharedScHpCoeffs_ = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, 350.0f, 0.70710678f);
        sharedScLpCoeffs_ = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate_, 2500.0f, 0.70710678f);

        juce::dsp::ProcessSpec monoSpec {};
        monoSpec.sampleRate       = sampleRate_;
        monoSpec.maximumBlockSize = spec.maximumBlockSize;
        monoSpec.numChannels      = 1;

        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            scHighPass_[ch].prepare(monoSpec);
            scHighPass_[ch].coefficients = sharedScHpCoeffs_;

            scLowPass_[ch].prepare(monoSpec);
            scLowPass_[ch].coefficients = sharedScLpCoeffs_;
        }

        juce::dsp::ProcessSpec multiSpec {};
        multiSpec.sampleRate       = sampleRate_;
        multiSpec.maximumBlockSize = spec.maximumBlockSize;
        multiSpec.numChannels      = static_cast<juce::uint32>(kMaxChannels);

        crossoverLowMid_.prepare(multiSpec);
        crossoverLowMid_.setCutoffFrequency(350.0f);

        crossoverMidHigh_.prepare(multiSpec);
        crossoverMidHigh_.setCutoffFrequency(2500.0f);

        // Allpass Linkwitz-Riley a 2.5 kHz para alinear la fase de la banda grave con Mid+High
        lowBandPhaseAlign_.prepare(multiSpec);
        lowBandPhaseAlign_.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
        lowBandPhaseAlign_.setCutoffFrequency(2500.0f);

        cachedScLow_  = 0.2269f;
        cachedScHigh_ = 0.5293f;
        currentHpHz_  = 350.0f;
        currentLpHz_  = 2500.0f;

        reset();
    }

    void reset() noexcept
    {
        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            scHighPass_[ch].reset();
            scLowPass_[ch].reset();
        }

        crossoverLowMid_.reset();
        crossoverMidHigh_.reset();
        lowBandPhaseAlign_.reset();

        scMeanSquare_  = 1.0e-9f;
        bandRmsSq_     = { 1.0e-9f, 1.0e-9f, 1.0e-9f };
        ambFloorSq_    = { 1.0e-9f, 1.0e-9f, 1.0e-9f };
        smoothedGainDb_ = { 0.0f, 0.0f, 0.0f };
        lastMidGainDb_ = 0.0f;
    }

    void updateParameters(const AmbienceGuardParams& params) noexcept
    {
        params_ = params;

        // Actualizar coeficientes juce::dsp::IIR en sitio (sin heap allocation) solo si cambian SCLow/SCHigh
        if (std::abs(params_.scLow - cachedScLow_) > 1.0e-5f
            || std::abs(params_.scHigh - cachedScHigh_) > 1.0e-5f)
        {
            cachedScLow_  = params_.scLow;
            cachedScHigh_ = params_.scHigh;

            const float nyquistLimit = static_cast<float>(sampleRate_ * 0.47);
            const float hpHz = juce::jlimit(20.0f, nyquistLimit - 200.0f, mapScNormToHz(cachedScLow_));
            const float lpHz = juce::jlimit(hpHz + 120.0f, nyquistLimit, mapScNormToHz(cachedScHigh_));

            currentHpHz_ = hpHz;
            currentLpHz_ = lpHz;

            if (sharedScHpCoeffs_ != nullptr)
            {
                *sharedScHpCoeffs_ = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
                    sampleRate_, hpHz, 0.70710678f);
            }

            if (sharedScLpCoeffs_ != nullptr)
            {
                *sharedScLpCoeffs_ = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
                    sampleRate_, lpHz, 0.70710678f);
            }

            crossoverLowMid_.setCutoffFrequency(hpHz);
            crossoverMidHigh_.setCutoffFrequency(lpHz);
            lowBandPhaseAlign_.setCutoffFrequency(lpHz);
        }

        // Constantes de tiempo del detector RMS Sidechain (Ataque 12 ms, DetRel = 0.458333 -> ~295 ms)
        detAtkCoeff_ = calcOnePoleCoeff(0.012f);
        const float detRelSec = 0.040f + juce::jlimit(0.0f, 1.0f, params_.detRel) * 0.556f;
        detRelCoeff_ = calcOnePoleCoeff(detRelSec);

        // Estimador asimétrico del piso de ruido ambiental estacionario (sube lento ~1.5 s, cae rápido ~110 ms)
        ambRiseCoeff_ = calcOnePoleCoeff(1.500f);
        ambFallCoeff_ = calcOnePoleCoeff(0.110f);

        // Balística de aplicación de ganancia (Release 0.0 -> 35 ms ágil .. 1.0 -> 420 ms)
        gainAtkCoeff_ = calcOnePoleCoeff(0.008f);
        const float relSec = 0.035f + juce::jlimit(0.0f, 1.0f, params_.release) * 0.385f;
        gainRelCoeff_ = calcOnePoleCoeff(relSec);

        // Umbral Target (0.388889 equivale a -24.0 dBFS RMS / -14.0 LUFS en escala [-38..-2])
        thresholdDb_ = -38.0f + juce::jlimit(0.0f, 1.0f, params_.target) * 36.0f;
    }

    void processBlock(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        if (numChannels <= 0 || numSamples <= 0)
            return;

        if (params_.bypass)
        {
            lastMidGainDb_ = 0.0f;
            return;
        }

        const size_t activeCh = static_cast<size_t>(juce::jmin(numChannels, static_cast<int>(kMaxChannels)));
        const float invCh     = 1.0f / static_cast<float>(activeCh);

        std::array<float*, kMaxChannels> chPtrs {};
        for (size_t ch = 0; ch < activeCh; ++ch)
            chPtrs[ch] = buffer.getWritePointer(static_cast<int>(ch));

        std::array<float, kMaxChannels> lowSamples {};
        std::array<float, kMaxChannels> midSamples {};
        std::array<float, kMaxChannels> highSamples {};

        for (int i = 0; i < numSamples; ++i)
        {
            float scEnergySum   = 0.0f;
            float lowEnergySum  = 0.0f;
            float midEnergySum  = 0.0f;
            float highEnergySum = 0.0f;

            // 1. Filtrado Sidechain IIR (350 Hz - 2.5 kHz) y separación Multibanda por canal
            for (size_t ch = 0; ch < activeCh; ++ch)
            {
                const float x = chPtrs[ch][i];

                // Detector Sidechain IIR estricto: High-Pass (SCLow) -> Low-Pass (SCHigh)
                const float scHp = scHighPass_[ch].processSample(x);
                const float scBp = scLowPass_[ch].processSample(scHp);
                scEnergySum += scBp * scBp;

                if (params_.bandMod)
                {
                    float lowRaw  = 0.0f;
                    float midHigh = 0.0f;
                    crossoverLowMid_.processSample(static_cast<int>(ch), x, lowRaw, midHigh);

                    const float lowAligned = lowBandPhaseAlign_.processSample(static_cast<int>(ch), lowRaw);

                    float mid  = 0.0f;
                    float high = 0.0f;
                    crossoverMidHigh_.processSample(static_cast<int>(ch), midHigh, mid, high);

                    lowSamples[ch]  = lowAligned;
                    midSamples[ch]  = mid;
                    highSamples[ch] = high;

                    lowEnergySum  += lowAligned * lowAligned;
                    midEnergySum  += mid * mid;
                    highEnergySum += high * high;
                }
            }

            const float scMeanSq = scEnergySum * invCh;
            const float scCoeff  = (scMeanSq > scMeanSquare_) ? detAtkCoeff_ : detRelCoeff_;
            scMeanSquare_ = scCoeff * scMeanSquare_ + (1.0f - scCoeff) * scMeanSq;
            const float scDb = 10.0f * std::log10(juce::jmax(scMeanSquare_, 1.0e-12f));

            if (params_.bandMod)
            {
                // 2A. Modo Multibanda (BandMod = 1): 3 bandas independientes con protección táctica central
                const std::array<float, 3> instSq {
                    lowEnergySum * invCh,
                    scMeanSq, // La banda media se gobierna estrictamente por el Sidechain IIR 350 Hz - 2.5 kHz
                    highEnergySum * invCh
                };

                for (size_t b = 0; b < 3; ++b)
                {
                    const float dCoeff = (instSq[b] > bandRmsSq_[b]) ? detAtkCoeff_ : detRelCoeff_;
                    bandRmsSq_[b] = dCoeff * bandRmsSq_[b] + (1.0f - dCoeff) * instSq[b];

                    const float aCoeff = (bandRmsSq_[b] > ambFloorSq_[b]) ? ambRiseCoeff_ : ambFallCoeff_;
                    ambFloorSq_[b] = aCoeff * ambFloorSq_[b] + (1.0f - aCoeff) * bandRmsSq_[b];

                    const float bandDb = (b == 1)
                        ? scDb
                        : 10.0f * std::log10(juce::jmax(bandRmsSq_[b], 1.0e-12f));

                    // Índice de estacionariedad [0..1]: cercano a 1 cuando el sonido es ruido constante de fondo
                    const float stationarity = juce::jlimit(0.0f, 1.0f, ambFloorSq_[b] / (bandRmsSq_[b] + 1.0e-11f));
                    const bool isTacticalMidBand = (b == 1);

                    const float targetGainDb = computeBandDownwardGainDb(bandDb, stationarity, isTacticalMidBand);
                    const float gCoeff = (targetGainDb < smoothedGainDb_[b]) ? gainAtkCoeff_ : gainRelCoeff_;
                    smoothedGainDb_[b] = gCoeff * smoothedGainDb_[b] + (1.0f - gCoeff) * targetGainDb;
                }

                const float gLow  = juce::Decibels::decibelsToGain(smoothedGainDb_[0]);
                const float gMid  = juce::Decibels::decibelsToGain(smoothedGainDb_[1]);
                const float gHigh = juce::Decibels::decibelsToGain(smoothedGainDb_[2]);

                for (size_t ch = 0; ch < activeCh; ++ch)
                    chPtrs[ch][i] = lowSamples[ch] * gLow + midSamples[ch] * gMid + highSamples[ch] * gHigh;
            }
            else
            {
                // 2B. Modo Banda Completa gobernado por el Sidechain IIR 350 Hz - 2.5 kHz (BandMod = 0)
                const float aCoeff = (scMeanSquare_ > ambFloorSq_[1]) ? ambRiseCoeff_ : ambFallCoeff_;
                ambFloorSq_[1] = aCoeff * ambFloorSq_[1] + (1.0f - aCoeff) * scMeanSquare_;

                const float stationarity = juce::jlimit(0.0f, 1.0f, ambFloorSq_[1] / (scMeanSquare_ + 1.0e-11f));
                const float targetGainDb = computeBandDownwardGainDb(scDb, stationarity, true);

                const float gCoeff = (targetGainDb < smoothedGainDb_[1]) ? gainAtkCoeff_ : gainRelCoeff_;
                smoothedGainDb_[1] = gCoeff * smoothedGainDb_[1] + (1.0f - gCoeff) * targetGainDb;

                const float gLin = juce::Decibels::decibelsToGain(smoothedGainDb_[1]);
                for (size_t ch = 0; ch < activeCh; ++ch)
                    chPtrs[ch][i] *= gLin;
            }
        }

        crossoverLowMid_.snapToZero();
        crossoverMidHigh_.snapToZero();
        lowBandPhaseAlign_.snapToZero();

        lastMidGainDb_ = smoothedGainDb_[1];
    }

    [[nodiscard]] float getCurrentHpHz() const noexcept      { return currentHpHz_; }
    [[nodiscard]] float getCurrentLpHz() const noexcept      { return currentLpHz_; }
    [[nodiscard]] float getCurrentGainDb() const noexcept    { return lastMidGainDb_; }

private:
    [[nodiscard]] float calcOnePoleCoeff(float timeSeconds) const noexcept
    {
        const float safeTime = juce::jmax(0.0005f, timeSeconds);
        return std::exp(-1.0f / (safeTime * static_cast<float>(sampleRate_)));
    }

    /**
     * Computa la reducción selectiva (Downward Compression suave) en dB:
     *  - Atenúa ruido estacionario constante (viento, motores, lluvia) según AmbGuar.
     *  - Comprime suavemente excesos sobre el umbral Target (0.388889).
     *  - Si Distant = 1, protege eventos dinámicos no estacionarios (pasos/voces) en 350 Hz - 2.5 kHz.
     */
    [[nodiscard]] float computeBandDownwardGainDb(float levelDb, float stationarity, bool isTacticalMidBand) const noexcept
    {
        const float guardDepth = juce::jlimit(0.0f, 1.0f, params_.ambGuar);
        float totalReductionDb = 0.0f;

        // 1. Supresión selectiva del suelo estacionario cuando el ruido persiste por encima de -52 dBFS
        if (levelDb > -52.0f && guardDepth > 0.001f)
        {
            const float floorExcessNorm = juce::jlimit(0.0f, 1.0f, (levelDb - (-52.0f)) / 24.0f);
            float effectiveStationarity = stationarity * stationarity;

            // Si Distant=1 y estamos en la banda táctica (350 Hz - 2.5 kHz), protegemos sonidos dinámicos
            // (pasos/voces tienen baja estacionariedad frente al zumbido continuo)
            if (isTacticalMidBand && params_.distant)
                effectiveStationarity *= 0.45f;

            const float maxAmbientDuckDb = isTacticalMidBand ? -3.5f : -6.0f;
            totalReductionDb += maxAmbientDuckDb * guardDepth * floorExcessNorm * effectiveStationarity;
        }

        // 2. Downward Compression suave con codo blando (Soft-Knee 6 dB) sobre el umbral Target
        constexpr float kKneeWidthDb = 6.0f;
        constexpr float kDownwardSlope = 0.50f; // Ratio suave 2:1

        const float overDb = levelDb - thresholdDb_;
        if (overDb > -0.5f * kKneeWidthDb)
        {
            float compressedOverDb = 0.0f;
            if (overDb < 0.5f * kKneeWidthDb)
            {
                const float x = overDb + 0.5f * kKneeWidthDb;
                compressedOverDb = (x * x) / (2.0f * kKneeWidthDb);
            }
            else
            {
                compressedOverDb = overDb;
            }

            // En Distant=1 la banda media preserva mayor margen dinámico para pasos lejanos
            const float bandSlopeScale = (isTacticalMidBand && params_.distant) ? 0.70f : 1.0f;
            totalReductionDb -= compressedOverDb * kDownwardSlope * bandSlopeScale * juce::jmax(0.35f, guardDepth);
        }

        return juce::jlimit(-15.0f, 0.0f, totalReductionDb);
    }

    double sampleRate_ = 48000.0;
    AmbienceGuardParams params_ {};

    juce::dsp::IIR::Coefficients<float>::Ptr sharedScHpCoeffs_;
    juce::dsp::IIR::Coefficients<float>::Ptr sharedScLpCoeffs_;
    std::array<juce::dsp::IIR::Filter<float>, kMaxChannels> scHighPass_;
    std::array<juce::dsp::IIR::Filter<float>, kMaxChannels> scLowPass_;

    juce::dsp::LinkwitzRileyFilter<float> crossoverLowMid_;
    juce::dsp::LinkwitzRileyFilter<float> crossoverMidHigh_;
    juce::dsp::LinkwitzRileyFilter<float> lowBandPhaseAlign_;

    float cachedScLow_  = -1.0f;
    float cachedScHigh_ = -1.0f;
    float currentHpHz_  = 350.0f;
    float currentLpHz_  = 2500.0f;

    float detAtkCoeff_  = 0.99f;
    float detRelCoeff_  = 0.999f;
    float ambRiseCoeff_ = 0.9999f;
    float ambFallCoeff_ = 0.999f;
    float gainAtkCoeff_ = 0.99f;
    float gainRelCoeff_ = 0.999f;
    float thresholdDb_  = -24.0f;

    float scMeanSquare_ = 1.0e-9f;
    std::array<float, 3> bandRmsSq_      { 1.0e-9f, 1.0e-9f, 1.0e-9f };
    std::array<float, 3> ambFloorSq_     { 1.0e-9f, 1.0e-9f, 1.0e-9f };
    std::array<float, 3> smoothedGainDb_ { 0.0f, 0.0f, 0.0f };
    float lastMidGainDb_ = 0.0f;
};

} // namespace dash::dsp
