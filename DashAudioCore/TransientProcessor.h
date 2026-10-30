#pragma once

#include <memory>
#include <vector>

namespace dash::core
{

/**
 * Modo de selección de canales a procesar por el Supresor de Transitorios.
 */
enum class TransientChannelMode
{
    AllChannels,       // Procesa todos los canales activos
    CenterOrMidOnly,   // Procesa el canal central (5.1/7.1) o la componente Mid en estéreo
    FrontChannelsOnly  // Procesa canales frontales (L, R, C) excluyendo surround
};

/**
 * Parámetros del Supresor de Transitorios Impulsivos (TransientProcessor).
 *
 * Mantiene compatibilidad total con la configuración táctica previa (`gun`, `steps`, etc.)
 * y habilita parámetros acústicos avanzados para ajuste milimétrico de umbrales y envolventes.
 */
struct TransientParameters
{
    // Parámetros tácticos canónicos (compatibilidad hacia atrás)
    float gun     = 0.45f; // [0.0 .. 1.0] Ganancia objetivo del pico (0.45 => ~ -7 dB)
    float steps   = 0.0f;  // [0.0 .. 1.0] Factor de preservación de micro-transitorios
    bool  front   = true;  // [0 / 1] Aislar canal central (o Mid en 2.0)
    float restore = 0.0f;  // [0.0 .. 1.0] Compensación de cuerpo post-transitorio
    float release = 0.35f; // [0.0 .. 1.0] Relajación del seguidor de envolvente
    bool  bypass  = false;

    // Nuevos parámetros de ingeniería acústica avanzada
    float frequencySplitHz     = 280.0f; // [150.0 .. 1200.0 Hz] Frecuencia de corte Linkwitz-Riley 4th order
    float sensitivity          = 0.50f;  // [0.0 .. 1.0] Sensibilidad de detección de cresta
    float attackMs             = 0.20f;  // [0.05 .. 10.0 ms] Tiempo de ataque de atenuación
    float releaseMs            = 45.0f;  // [5.0 .. 250.0 ms] Tiempo de relajación de atenuación
    float maxAttenuationDb     = -6.94f; // [-24.0 .. 0.0 dB] Atenuación máxima para transitorios severos
    bool  stereoLinking        = true;   // true = vincular atenuación L/R para preservar panorama estéreo
    TransientChannelMode channelMode = TransientChannelMode::CenterOrMidOnly;
    bool  useAdvancedMode      = false;  // true = usar parámetros acústicos; false = compatibilidad canónica
};

/**
 * Supresor de Transitorios de Disparo e Impactos Acústicos (TransientProcessor).
 *
 * Principios operativos y matemáticos:
 *  - Crossover Linkwitz-Riley de 4.º orden (24 dB/octava): divide la señal en banda baja ("Body")
 *    y banda alta ("Crack") sumando con respuesta plana de amplitud y fase linealizada.
 *  - Detector de relación de cresta asimétrico:
 *      CrestRatio = EnvFast / (EnvSlow + epsilon)
 *    Distingue transitorios impulsivos abruptos (disparos) de micro-transitorios tácticos
 *    de menor energía (pasos, recargas de cargador, casquillos) para preservarlos intactos.
 *  - Atenuación confinada a la banda alta sin mutilar la energía de graves del cuerpo sonoro.
 *  - Enlace estéreo (Stereo Linking) para evitar alteraciones en la localización acústica lateral.
 *  - No efectúa clasificación semántica de audio (no asume "saber" qué es un arma); opera
 *    estrictamente mediante procesamiento determinista de señales de alto factor de cresta.
 *  - Cero asignaciones dinámicas en process().
 */
class TransientProcessor
{
public:
    static constexpr int   kMaxChannels          = 8;
    static constexpr float kDefaultSplitHz       = 280.0f;
    static constexpr float kMinSplitHz           = 150.0f;
    static constexpr float kMaxSplitHz           = 1200.0f;

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
    [[nodiscard]] const TransientParameters& getParameters() const noexcept;

    /** Procesa un bloque mono en sitio. */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

    [[nodiscard]] float getLastReductionDb() const noexcept;
    [[nodiscard]] float getCurrentSplitHz() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
