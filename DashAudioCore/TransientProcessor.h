#pragma once

#include <memory>
#include <vector>

namespace dash::core
{

struct TransientParameters
{
    float gun     = 0.45f; // [0.0 .. 1.0] Ganancia objetivo del pico de disparo (0.45 => -6.94 dB ~ -7 dB)
    float steps   = 0.0f;  // [0.0 .. 1.0] Protección adicional contra micro-transitorios de pasos
    bool  front   = true;  // [0 / 1] Aislar esfera frontal / Canal Central en 5.1 y 7.1 (o Mid en 2.0)
    float restore = 0.0f;  // [0.0 .. 1.0] Compensación suave post-transitorio
    float release = 0.35f; // [0.0 .. 1.0] Relajación ajustable del seguidor de envolvente
    bool  bypass  = false;
};

/**
 * Supresor de Transitorios de Disparo ("Gun Crack") en Tiempo Real (TransientProcessor).
 *
 * Separa "Gun Body" (< 280 Hz) de "Gun Crack" (>= 280 Hz) mediante crossover Linkwitz-Riley
 * de 4.º orden y atenúa únicamente los picos impulsivos agudos en el canal central/frontal.
 *
 * API pública C++20 estándar libre de dependencias de JUCE o VST.
 * Cero asignaciones dinámicas dentro de process().
 */
class TransientProcessor
{
public:
    static constexpr int   kMaxChannels          = 8;
    static constexpr float kBodyCrackCrossoverHz = 280.0f;

    TransientProcessor();
    ~TransientProcessor();

    TransientProcessor(TransientProcessor&&) noexcept;
    TransientProcessor& operator=(TransientProcessor&&) noexcept;

    TransientProcessor(const TransientProcessor&) = delete;
    TransientProcessor& operator=(const TransientProcessor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void setCenterChannelIndex(int centerChannelIdx) noexcept;
    void reset() noexcept;
    void updateParameters(const TransientParameters& params) noexcept;

    /** Procesa un bloque mono en sitio (API estándar C++). */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1.0, 2.0, 5.1, 7.1) en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    [[nodiscard]] float getLastReductionDb() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
