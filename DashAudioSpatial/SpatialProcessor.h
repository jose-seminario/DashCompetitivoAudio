#pragma once

#include "BinauralRenderer.h"
#include "ChannelLayout.h"
#include "ChannelMapping.h"
#include "HRIRDataset.h"
#include "HRTFProfile.h"

#include <memory>
#include <string>

namespace dash::spatial
{

struct SpatialProcessorParameters
{
    float soundstageWidth       = 1.0f;   // [0.5 .. 2.0] Factor de apertura y anchura estéreo espacial
    float apparentDistanceM     = 1.4f;   // [0.5 .. 5.0 m] Distancia acústica aparente
    float frontBackSeparation   = 1.0f;   // [0.0 .. 1.5] Claridad en la discriminación frente/detrás
    float elevationEmphasis     = 0.0f;   // [-6.0 .. +6.0 dB] Énfasis acústico en altavoces de techo
    BinauralRendererParameters rendererParams {};
    bool  bypass                = false;
};

/**
 * Procesador Espacial y Orquestador de Audio Tridimensional (SpatialProcessor).
 *
 * Utiliza el Modelo Esférico Woodworth con Escalado Paramétrico de Cabeza (Woodworth Spherical
 * Model with Parametric Head Scaling) y filtrado de muesca de elevación aproximado (Approximated
 * Pinna Notch Filtering) para renderizado binaural continuo de latencia cero en auriculares.
 * Coordina la negociación de layout, el mapeo de canales, el dataset HRIR y el renderizador binaural.
 */
class SpatialProcessor
{
public:
    static constexpr size_t kMaxChannels = ChannelLayout::kMaxChannels;

    SpatialProcessor();
    ~SpatialProcessor();

    SpatialProcessor(SpatialProcessor&&) noexcept;
    SpatialProcessor& operator=(SpatialProcessor&&) noexcept;

    SpatialProcessor(const SpatialProcessor&) = delete;
    SpatialProcessor& operator=(const SpatialProcessor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize);
    void reset() noexcept;

    /**
     * Configura el layout de altavoces de entrada (ej: 7.1 o 7.1.4) y sintetiza o carga el dataset HRIR.
     */
    bool configureLayout(const ChannelLayout& inputLayout);

    /**
     * Carga un dataset HRIR preconfigurado fuera del hilo de audio.
     */
    bool loadDataset(const HRIRDataset& dataset);

    void updateParameters(const SpatialProcessorParameters& params) noexcept;
    [[nodiscard]] const SpatialProcessorParameters& getParameters() const noexcept;

    /**
     * Procesa un bloque multicanal posicional (hasta 16 canales) y genera la salida binaural estéreo.
     */
    void processBinaural(
        const float* const* inputChannels,
        int numChannels,
        float* outputLeft,
        float* outputRight,
        int numSamples) noexcept;

    [[nodiscard]] const ChannelLayout& getInputLayout() const noexcept { return inputLayout_; }
    [[nodiscard]] const HRTFProfile& getActiveProfile() const noexcept { return hrirDataset_.getProfile(); }
    [[nodiscard]] bool isSpatialActive() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    ChannelLayout inputLayout_;
    HRIRDataset   hrirDataset_;
    ChannelMapping mapper_;
    BinauralRenderer renderer_;
};

} // namespace dash::spatial
