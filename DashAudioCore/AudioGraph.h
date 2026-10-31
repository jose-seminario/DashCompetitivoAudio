#pragma once

#include "AudioConstants.h"
#include "AudioGraphParameters.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Cadena Canónica de Procesamiento DSP en Tiempo Real (AudioGraph).
 * Soporta de 1 a 16 canales (kMaxSupportedChannels = 16).
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

    /**
     * Prepara e inicializa todos los procesadores de la cadena.
     */
    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);

    /**
     * Reinicia el estado interno de todos los filtros y detectores.
     */
    void reset() noexcept;

    /**
     * Actualiza los parámetros de todos los procesadores de la cadena.
     */
    void updateParameters(const AudioGraphParameters& parameters) noexcept;

    [[nodiscard]] const AudioGraphParameters& getParameters() const noexcept;

    /**
     * Procesa un bloque de audio multicanal planar (1 a 16 canales) en sitio.
     */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    /** Procesa un buffer mono continuo en sitio. */
    void process(float* buffer, int numSamples) noexcept;

    /** Procesa un std::vector<float> mono en sitio sin reasignaciones. */
    void process(std::vector<float>& buffer) noexcept;

    /** Activa o desactiva el bypass global de la cadena completa. */
    void setBypass(bool enabled) noexcept;

    [[nodiscard]] bool isBypassed() const noexcept;

    // Métricas y telemetría de monitoreo en tiempo real
    [[nodiscard]] float getLufs() const noexcept;
    [[nodiscard]] float getDynamicsGainDb() const noexcept;
    [[nodiscard]] float getTransientReductionDb() const noexcept;
    [[nodiscard]] float getAmbienceGainDb() const noexcept;
    [[nodiscard]] float getStationarityIndex() const noexcept;
    [[nodiscard]] bool  isSpatialActive() const noexcept;
    [[nodiscard]] float getLimiterReductionDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
