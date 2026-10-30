#pragma once

#include "ChannelLayout.h"
#include "ConvolutionEngine.h"
#include "HRIRDataset.h"

#include <memory>
#include <vector>

namespace dash::spatial
{

struct BinauralRendererParameters
{
    float masterGainDb       = 0.0f;   // [-12.0 .. +12.0 dB] Ganancia master de salida binaural
    float lfeGainDb          = 0.0f;   // [-12.0 .. +6.0 dB] Ganancia del canal subwoofer en auriculares
    float crossfeedAmount    = 0.0f;   // [0.0 .. 1.0] Alimentación cruzada de frecuencias graves (Bauer crossfeed)
    bool  bypassLfeHrtf      = true;   // true = LFE elude HRTF para prevenir comb-filtering en subgraves
    bool  bypass             = false;
};

/**
 * Renderizador Binaural Multicanal de Alta Definición (BinauralRenderer).
 *
 * Transforma un flujo multicanal posicional (hasta 16 canales) en una mezcla
 * estéreo tridimensional binaural para auriculares.
 *
 * Características:
 *  - Tratamiento dedicado de LFE: elude la convolución HRTF para evitar comb-filtering
 *    y cancelaciones de fase acústicas en bajas frecuencias.
 *  - Cero asignaciones dinámicas en tiempo real.
 *  - Cero latencia añadida.
 */
class BinauralRenderer
{
public:
    BinauralRenderer();
    ~BinauralRenderer();

    BinauralRenderer(BinauralRenderer&&) noexcept;
    BinauralRenderer& operator=(BinauralRenderer&&) noexcept;

    BinauralRenderer(const BinauralRenderer&) = delete;
    BinauralRenderer& operator=(const BinauralRenderer&) = delete;

    void prepare(double sampleRate, int maximumBlockSize);
    void reset() noexcept;

    /**
     * Carga un dataset HRIR completo para el layout especificado.
     */
    bool setDataset(const HRIRDataset& dataset, const ChannelLayout& layout);

    void updateParameters(const BinauralRendererParameters& params) noexcept;
    [[nodiscard]] const BinauralRendererParameters& getParameters() const noexcept;

    /**
     * Renderiza un bloque multicanal planar (hasta 16 canales) en 2 canales binaurales estéreo.
     */
    void render(
        const float* const* inputChannels,
        size_t numChannels,
        float* outputLeft,
        float* outputRight,
        int numSamples) noexcept;

    [[nodiscard]] const ChannelLayout& getLayout() const noexcept { return layout_; }
    [[nodiscard]] bool isReady() const noexcept { return isConfigured_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    ChannelLayout layout_;
    bool isConfigured_ = false;
};

} // namespace dash::spatial
