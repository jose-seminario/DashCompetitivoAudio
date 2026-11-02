#include "SpatialProcessor.h"

#include <algorithm>
#include <array>
#include <vector>

namespace dash::spatial
{

struct SpatialProcessor::Impl
{
    double sampleRate   = 48000.0;
    int    maxBlockSize = 512;
    SpatialProcessorParameters params {};

    std::array<std::vector<float>, kMaxChannels> mappedScratch;
    std::array<float*, kMaxChannels> mappedPointers {};

    void prepareBuffers(int blockSize)
    {
        maxBlockSize = std::max(1, blockSize);
        for (size_t ch = 0; ch < kMaxChannels; ++ch)
        {
            mappedScratch[ch].assign(static_cast<size_t>(maxBlockSize), 0.0f);
            mappedPointers[ch] = mappedScratch[ch].data();
        }
    }
};

SpatialProcessor::SpatialProcessor()
    : impl_(std::make_unique<Impl>())
{
    inputLayout_ = ChannelLayout::surround7_1();
}

SpatialProcessor::~SpatialProcessor() = default;
SpatialProcessor::SpatialProcessor(SpatialProcessor&&) noexcept = default;
SpatialProcessor& SpatialProcessor::operator=(SpatialProcessor&&) noexcept = default;

void SpatialProcessor::prepare(double sampleRate, int maximumBlockSize)
{
    impl_->sampleRate   = (sampleRate > 8000.0) ? sampleRate : 48000.0;
    impl_->maxBlockSize = std::max(1, maximumBlockSize);
    impl_->prepareBuffers(impl_->maxBlockSize);

    renderer_.prepare(impl_->sampleRate, impl_->maxBlockSize);
    configureLayout(inputLayout_);
}

void SpatialProcessor::reset() noexcept
{
    renderer_.reset();
    for (size_t ch = 0; ch < kMaxChannels; ++ch)
        std::fill(impl_->mappedScratch[ch].begin(), impl_->mappedScratch[ch].end(), 0.0f);
}

bool SpatialProcessor::configureLayout(const ChannelLayout& inputLayout)
{
    inputLayout_ = inputLayout;
    mapper_.configure(inputLayout_, inputLayout_);

    // Síntesis analítica de referencia HRIR basada en el modelo esférico Woodworth
    hrirDataset_.loadSyntheticReference(inputLayout_, impl_->sampleRate, 256);
    renderer_.setDataset(hrirDataset_, inputLayout_);

    return true;
}

bool SpatialProcessor::loadDataset(const HRIRDataset& dataset)
{
    hrirDataset_.setProfile(dataset.getProfile());
    const size_t numCh = inputLayout_.getNumChannels();

    for (size_t ch = 0; ch < numCh; ++ch)
    {
        const auto* pair = dataset.getImpulsePair(ch);
        if (pair != nullptr && pair->isValid())
        {
            hrirDataset_.setImpulsePair(
                ch,
                pair->leftEar.data(),
                pair->rightEar.data(),
                pair->leftEar.size());
        }
    }

    return renderer_.setDataset(hrirDataset_, inputLayout_);
}

void SpatialProcessor::updateParameters(const SpatialProcessorParameters& params) noexcept
{
    impl_->params = params;
    renderer_.updateParameters(params.rendererParams);
}

const SpatialProcessorParameters& SpatialProcessor::getParameters() const noexcept
{
    return impl_->params;
}

bool SpatialProcessor::isSpatialActive() const noexcept
{
    return inputLayout_.isSpatialized() && !impl_->params.bypass;
}

void SpatialProcessor::processBinaural(
    const float* const* inputChannels,
    int numChannels,
    float* outputLeft,
    float* outputRight,
    int numSamples) noexcept
{
    if (inputChannels == nullptr || outputLeft == nullptr || outputRight == nullptr || numSamples <= 0)
        return;

    const size_t activeCh = static_cast<size_t>(std::clamp(numChannels, 1, static_cast<int>(kMaxChannels)));

    int samplesProcessed = 0;
    while (samplesProcessed < numSamples)
    {
        const int chunkSize = std::min(numSamples - samplesProcessed, impl_->maxBlockSize);

        // Reasignar punteros del buffer de memoria scratch para el fragmento actual
        for (size_t ch = 0; ch < activeCh; ++ch)
        {
            if (inputChannels[ch] != nullptr)
                std::copy(inputChannels[ch] + samplesProcessed, inputChannels[ch] + samplesProcessed + chunkSize, impl_->mappedPointers[ch]);
            else
                std::fill(impl_->mappedPointers[ch], impl_->mappedPointers[ch] + chunkSize, 0.0f);
        }

        // Procesar modificaciones de anchura de escenario y separación frontal/trasera
        if (std::abs(impl_->params.soundstageWidth - 1.0f) > 1.0e-3f ||
            std::abs(impl_->params.frontBackSeparation - 1.0f) > 1.0e-3f)
        {
            const float widthMod = impl_->params.soundstageWidth;
            const float fbMod    = impl_->params.frontBackSeparation;

            for (size_t ch = 0; ch < activeCh; ++ch)
            {
                const auto& spk = inputLayout_.getSpeaker(ch);
                float* data = impl_->mappedPointers[ch];

                // Escalar canales laterales por anchura de escenario
                if (spk.role == ChannelRole::SideLeft || spk.role == ChannelRole::SideRight)
                {
                    for (int i = 0; i < chunkSize; ++i)
                        data[i] *= widthMod;
                }
                // Diferenciación frontal vs trasera
                else if (spk.role == ChannelRole::BackLeft || spk.role == ChannelRole::BackRight)
                {
                    for (int i = 0; i < chunkSize; ++i)
                        data[i] *= fbMod;
                }
            }
        }

        // Renderizar la mezcla binaural estéreo de alta fidelidad (Wet)
        renderer_.render(
            impl_->mappedPointers.data(),
            activeCh,
            outputLeft + samplesProcessed,
            outputRight + samplesProcessed,
            chunkSize);

        // Flujo de Procesamiento Binaural, Mezcla Dry/Wet e ILD Limiter (Cero Latencia)
        float* outL = outputLeft + samplesProcessed;
        float* outR = outputRight + samplesProcessed;

        // Punteros a los canales de entrada para el downmix estéreo nativo (Dry)
        auto getChannelPtr = [&](ChannelRole role, int fallbackIdx) -> const float* {
            const int idx = inputLayout_.getChannelIndex(role);
            const int ch = (idx >= 0) ? idx : fallbackIdx;
            if (ch >= 0 && ch < numChannels && inputChannels[ch] != nullptr)
                return inputChannels[ch] + samplesProcessed;
            return nullptr;
        };

        const float* ptrFL = getChannelPtr(ChannelRole::FrontLeft, 0);
        const float* ptrFR = getChannelPtr(ChannelRole::FrontRight, (numChannels > 1 ? 1 : 0));
        const float* ptrFC = getChannelPtr(ChannelRole::FrontCenter, (numChannels > 2 ? 2 : -1));
        const float* ptrBL = getChannelPtr(ChannelRole::BackLeft, (numChannels > 4 ? 4 : -1));
        const float* ptrBR = getChannelPtr(ChannelRole::BackRight, (numChannels > 5 ? 5 : -1));
        const float* ptrSL = getChannelPtr(ChannelRole::SideLeft, (numChannels > 6 ? 6 : -1));
        const float* ptrSR = getChannelPtr(ChannelRole::SideRight, (numChannels > 7 ? 7 : -1));

        const float mix = std::clamp(impl_->params.spatialMix, 0.0f, 1.0f);
        constexpr float kDownmixAtten = 0.7071f;

        for (int i = 0; i < chunkSize; ++i)
        {
            // Paso A (Wet/HRTF): Obtén la salida del motor 3D
            const float wetL = outL[i];
            const float wetR = outR[i];

            // Paso B (Dry/Nativo): Calcula el Downmix de los 8 canales a 2 (L y R)
            // sumando los canales traseros y laterales con atenuación de 0.7071f
            const float sFL = (ptrFL != nullptr) ? ptrFL[i] : 0.0f;
            const float sFR = (ptrFR != nullptr) ? ptrFR[i] : 0.0f;
            const float sFC = (ptrFC != nullptr) ? ptrFC[i] : 0.0f;
            const float sBL = (ptrBL != nullptr) ? ptrBL[i] : 0.0f;
            const float sBR = (ptrBR != nullptr) ? ptrBR[i] : 0.0f;
            const float sSL = (ptrSL != nullptr) ? ptrSL[i] : 0.0f;
            const float sSR = (ptrSR != nullptr) ? ptrSR[i] : 0.0f;

            const float dryL = sFL + kDownmixAtten * (sFC + sBL + sSL);
            const float dryR = sFR + kDownmixAtten * (sFC + sBR + sSR);

            // Paso C (Mezcla): Aplica el slider entre Wet (HRTF) y Dry (Nativo)
            float mixedL = (wetL * (1.0f - mix)) + (dryL * mix);
            float mixedR = (wetR * (1.0f - mix)) + (dryR * mix);

            // Paso D (ILD Limiter - La Solución):
            // Evita cancelaciones acústicas o silencio total (0 dB) por la espalda o laterales.
            // Si el HRTF atenúa el oído a 0.0f, se fuerza a un mínimo de 0.50f (50%).

            // 1. Piso de volumen mínimo ante disparos traseros o laterales (50% de la energía Dry)
            const float dryFloorL = 0.50f * std::abs(dryL);
            if (std::abs(mixedL) < dryFloorL)
            {
                const float signL = (std::abs(mixedL) > 1.0e-7f) ? (mixedL > 0.0f ? 1.0f : -1.0f) : (dryL >= 0.0f ? 1.0f : -1.0f);
                mixedL = signL * dryFloorL;
            }

            const float dryFloorR = 0.50f * std::abs(dryR);
            if (std::abs(mixedR) < dryFloorR)
            {
                const float signR = (std::abs(mixedR) > 1.0e-7f) ? (mixedR > 0.0f ? 1.0f : -1.0f) : (dryR >= 0.0f ? 1.0f : -1.0f);
                mixedR = signR * dryFloorR;
            }

            // 2. Límite de Diferencia de Nivel Interaural (ILD Limiter 50% contralateral):
            // Si un oído recibe sonido pero el contralateral cae a 0 por sombra de la cabeza,
            // garantiza al menos el 50% de nivel en el oído atenuado.
            const float magL = std::abs(mixedL);
            const float magR = std::abs(mixedR);

            if (magL > 1.0e-5f && magR < 0.50f * magL)
            {
                const float signR = (magR > 1.0e-7f) ? (mixedR > 0.0f ? 1.0f : -1.0f) : (dryR != 0.0f ? (dryR > 0.0f ? 1.0f : -1.0f) : (mixedL > 0.0f ? 1.0f : -1.0f));
                mixedR = signR * (0.50f * magL);
            }
            else if (magR > 1.0e-5f && magL < 0.50f * magR)
            {
                const float signL = (magL > 1.0e-7f) ? (mixedL > 0.0f ? 1.0f : -1.0f) : (dryL != 0.0f ? (dryL > 0.0f ? 1.0f : -1.0f) : (mixedR > 0.0f ? 1.0f : -1.0f));
                mixedL = signL * (0.50f * magR);
            }

            // Paso E (Salida): Asignación a los canales de salida estéreo (auriculares)
            outL[i] = mixedL;
            outR[i] = mixedR;
        }

        samplesProcessed += chunkSize;
    }
}

} // namespace dash::spatial
