#pragma once

#include "AudioConstants.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace dash::core
{

struct EQParameters
{
    float footstepsDb = 1.6f;  // [-3.0 .. +6.0 dB] Realce/recorte táctico de pasos (3.15 kHz / 6.3 kHz)
    float bassDb      = 1.5f;  // [-6.0 .. +6.0 dB] Control de graves (80 Hz .. 250 Hz)
    float preampDb    = 0.0f;  // Ganancia previa opcional en dB
    bool  bypass      = false;
};

/**
 * Procesador de Ecualización Paramétrica/Gráfica de 31 Bandas ISO (20 Hz - 20 kHz).
 * Soporta de 1 a 16 canales simultáneos (kMaxSupportedChannels = 16).
 */
class EQProcessor
{
public:
    static constexpr size_t kNumBands    = 31;
    static constexpr size_t kMaxChannels = kMaxSupportedChannels;

    static constexpr std::array<float, kNumBands> kIsoFrequencies {
        20.0f,    25.0f,    31.5f,    40.0f,    50.0f,    63.0f,    80.0f,    100.0f,
        125.0f,   160.0f,   200.0f,   250.0f,   315.0f,   400.0f,   500.0f,   630.0f,
        800.0f,   1000.0f,  1250.0f,  1600.0f,  2000.0f,  2500.0f,  3150.0f,  4000.0f,
        5000.0f,  6300.0f,  8000.0f,  10000.0f, 12500.0f, 16000.0f, 20000.0f
    };

    static constexpr std::array<const char*, kNumBands> kIsoFrequencyLabels {
        "20",    "25",    "31.5",  "40",    "50",    "63",    "80",    "100",
        "125",   "160",   "200",   "250",   "315",   "400",   "500",   "630",
        "800",   "1000",  "1250",  "1600",  "2000",  "2500",  "3150",  "4000",
        "5000",  "6300",  "8000",  "10000", "12500", "16000", "20000"
    };

    static constexpr std::array<double, kNumBands> kBaseCurveDb {
        -3.0, -2.0, -0.8,  0.5,  1.5,  2.2,  2.0,  1.5,
         1.2,  0.8,  0.2, -0.6, -1.4, -1.8, -1.5, -1.0,
        -0.5,  0.0,  0.3,  0.6,  1.0,  1.4,  1.6,  1.2,
         0.8,  2.2,  1.8,  0.7,  0.2, -0.5, -1.2
    };

    EQProcessor();
    ~EQProcessor();

    EQProcessor(EQProcessor&&) noexcept;
    EQProcessor& operator=(EQProcessor&&) noexcept;

    EQProcessor(const EQProcessor&) = delete;
    EQProcessor& operator=(const EQProcessor&) = delete;

    [[nodiscard]] static std::array<double, kNumBands> computeBandGainsDb(
        double footstepsDb = 1.6,
        double bassDb = 1.5) noexcept;

    void prepare(double sampleRate, int maximumBlockSize, int numChannels = 2);
    void reset() noexcept;
    void updateParameters(const EQParameters& params) noexcept;

    /** Procesa un bloque mono en sitio (API estándar C++). */
    void process(float* buffer, int numSamples) noexcept;

    /** Sobrecarga con std::vector<float> (sin asignaciones dinámicas). */
    void process(std::vector<float>& buffer) noexcept;

    /** Procesa un bloque multicanal planar (1 a 16 canales) en sitio. */
    void process(float* const* channelPointers, int numChannels, int numSamples) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dash::core
