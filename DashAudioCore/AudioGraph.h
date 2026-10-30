#pragma once

#include "AudioGraphParameters.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Cadena Canónica de Procesamiento DSP en Tiempo Real (AudioGraph).
 *
 * Orquesta secuencialmente los 5 procesadores centrales de DashAudioCore:
 *  1. Input / Headroom (Ajuste de ganancia inicial, -6 dB por defecto).
 *  2. DynamicsProcessor (Compresión híbrida Upward/Downward BS.1770-4).
 *  3. TransientProcessor (Supresión quirúrgica de Gun Crack en canal central/Mid).
 *  4. AmbienceProcessor (Atenuación de ruido estacionario y protección de 350 Hz - 2.5 kHz).
 *  5. EQProcessor (Ecualización acústica de 31 bandas ISO).
 *  6. LimiterProcessor (Limitador de techo de muestra exacta a -0.5 dBFS).
 *
 * Características:
 *  - Cero asignaciones de memoria dinámica dentro de process().
 *  - Libre de locks, operaciones de archivo o dependencias de UI.
 *  - Soporta mono (1.0), estéreo (2.0), 5.1 y 7.1 Surround.
 *  - Bypass general y bypass independiente por cada procesador.
 */
class AudioGraph
{
public:
    static constexpr size_t kMaxChannels = 8;

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
     * Procesa un bloque de audio multicanal planar en sitio.
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
    [[nodiscard]] float getLimiterReductionDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
