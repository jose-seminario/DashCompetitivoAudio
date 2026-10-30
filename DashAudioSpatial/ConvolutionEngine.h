#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace dash::spatial
{

/**
 * Motor de Convolución en Tiempo Real para Respuestas al Impulso (ConvolutionEngine).
 *
 * Principios:
 *  - Cero latencia añadida (convolución directa de tiempo de muestra exacta sin buffer de retardo FFT).
 *  - Cero asignaciones de memoria dinámica dentro de process().
 *  - Conmutación lock-free de filtros (Double-Buffering) para permitir cambio de perfiles
 *    HRIR en caliente sin pausas, cortes ni artefactos en el hilo de audio.
 *  - Soporta hasta kMaxChannels canales de entrada, convolucionando simultáneamente
 *    los pares izquierdo y derecho hacia la salida binaural.
 */
class ConvolutionEngine
{
public:
    static constexpr size_t kMaxChannels = 16;
    static constexpr size_t kMaxFilterLength = 1024;

    ConvolutionEngine();
    ~ConvolutionEngine();

    ConvolutionEngine(ConvolutionEngine&&) noexcept;
    ConvolutionEngine& operator=(ConvolutionEngine&&) noexcept;

    ConvolutionEngine(const ConvolutionEngine&) = delete;
    ConvolutionEngine& operator=(const ConvolutionEngine&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, size_t maxFilterLength = 256);
    void reset() noexcept;

    /**
     * Carga un par de respuestas al impulso en el banco de reserva fuera del hilo de audio.
     */
    bool loadFilterPair(
        size_t channelIndex,
        const float* leftEarIR,
        const float* rightEarIR,
        size_t filterLength);

    /**
     * Conmuta atómicamente el banco de filtros activo de forma lock-free.
     */
    void commitFilters() noexcept;

    /**
     * Convoluciona un canal multicanal planar y acumula el resultado en los buffers binaurales L y R.
     */
    void convolveAndAccumulate(
        size_t channelIndex,
        const float* input,
        float* outputLeft,
        float* outputRight,
        int numSamples) noexcept;

    [[nodiscard]] size_t getFilterLength() const noexcept;
    [[nodiscard]] bool hasFilters(size_t channelIndex) const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::spatial
