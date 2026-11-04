#pragma once

#include <DashAudioCore/ParametricEQProcessor.h>

#include <array>
#include <filesystem>
#include <string>

namespace dash::audio
{

/**
 * Configuración estéreo de 6 bandas Biquad para Equalizer APO.
 */
struct ApoChainConfig
{
    // Filtro opcional de dispositivo Equalizer APO, ej. "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
    std::string targetEndpointGuid;

    // Preamp global en dB [-18.0 .. +18.0 dB]
    double preampDb = 0.0;

    // Ganancias en dB de las 6 bandas tácticas [-18.0 .. +18.0 dB]:
    //  [0] Sub-Bajos    (LS 60 Hz,   Q=0.707)
    //  [1] Bajos        (LS 150 Hz,  Q=0.707)
    //  [2] Cuerpo       (PK 400 Hz,  Q=1.0)
    //  [3] Pasos Medios (PK 1500 Hz, Q=1.2)
    //  [4] Pasos Agudos (PK 3500 Hz, Q=1.2)
    //  [5] Disparos     (HS 8000 Hz, Q=0.707)
    std::array<double, dash::core::ParametricEQProcessor::kMaxBands> bandGainsDb {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0
    };

    // Bypass maestro
    bool bypass = false;
};

class ApoConfigInjector
{
public:
    /**
     * Ruta dinámica de config.txt en Equalizer APO (consulta HKLM\SOFTWARE\EqualizerAPO o %ProgramFiles%).
     */
    static std::filesystem::path getDefaultApoConfigPath();

    /**
     * Construye en memoria las directivas estéreo de Equalizer APO (Preamp + 6 filtros Biquad LS/PK/HS) con CRLF.
     */
    static std::string buildConfigContent(
        const ApoChainConfig& config = {},
        const std::filesystem::path& targetConfigFilePath = {});

    /**
     * Escribe el archivo config.txt de forma atómica y segura frente a lecturas concurrentes.
     */
    static bool writeConfigAtomically(
        const std::filesystem::path& configFilePath,
        const ApoChainConfig& config = {},
        std::string* outError = nullptr,
        bool* outPermissionDenied = nullptr);

    /**
     * Escribe un config.txt vacío de forma atómica.
     */
    static bool writeEmptyConfigAtomically(
        const std::filesystem::path& configFilePath,
        std::string* outError = nullptr,
        bool* outPermissionDenied = nullptr);

private:
    static bool writeRawPayloadAtomically(
        const std::filesystem::path& configFilePath,
        const std::string& payload,
        std::string* outError,
        bool* outPermissionDenied);
};

} // namespace dash::audio
