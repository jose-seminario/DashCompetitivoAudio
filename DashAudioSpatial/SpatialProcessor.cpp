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

        // Renderizar la mezcla binaural estéreo de alta fidelidad
        renderer_.render(
            impl_->mappedPointers.data(),
            activeCh,
            outputLeft + samplesProcessed,
            outputRight + samplesProcessed,
            chunkSize);

        samplesProcessed += chunkSize;
    }
}

} // namespace dash::spatial
