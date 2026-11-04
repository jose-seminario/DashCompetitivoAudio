#pragma once

#include "AudioConstants.h"
#include "AudioGraphParameters.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Cadena Estéreo Táctica en Tiempo Real (AudioGraph).
 * Flujo DSP: Entrada Estéreo (2ch) -> Preamp -> ParametricEQ (6 bandas) -> Limiter (-0.5 dBFS) -> Salida.
 */
class AudioGraph
{
public:
    static constexpr size_t kMaxChannels = kMaxSupportedChannels;

    AudioGraph();
    ~AudioGraph();

    AudioGraph(AudioGraph&&) noexcept;
    AudioGraph& operator=(AudioGraph&&) noexcept;

    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;

    void updateParameters(const AudioGraphParameters& parameters) noexcept;
    [[nodiscard]] const AudioGraphParameters& getParameters() const noexcept;

    void setBandGainDb(size_t bandIndex, float gainDb) noexcept;
    void setPreampDb(float preampDb) noexcept;

    /** Procesa un bloque estéreo planar (1 o 2 canales) en sitio sin asignaciones dinámicas. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;
    void process(float* buffer, int numSamples) noexcept;
    void process(std::vector<float>& buffer) noexcept;

    void setBypass(bool enabled) noexcept;
    [[nodiscard]] bool isBypassed() const noexcept;

    [[nodiscard]] float getLimiterReductionDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
