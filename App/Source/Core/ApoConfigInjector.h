#pragma once

#include <filesystem>
#include <string>

namespace dash::audio
{

struct ApoChainConfig
{
    // Filtro opcional de dispositivo Equalizer APO, ej. "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
    std::string targetEndpointGuid;

    // 1. Supresor de Transitorios (Disparos)
    bool enableTransientSuppressor = true;
    double gunVolume      = 0.450000;
    int    gunSteps       = 0;
    int    gunFrontOnly   = 1;
    int    gunRestore     = 0;
    int    gunBypass      = 0;

    // 2. Compresor Dinámico Híbrido de Sonoridad
    bool enableLoudnessCompressor = true;
    double compAmount     = 0.750000;
    double compTarget     = 0.277778; // Equivale a -18.0 LUFS en escala normalizada [-28..+8]
    double compRelease    = 0.5;
    double compDetRel     = 0.583333;
    double compLimRel     = 0.612903;
    double compScLow      = 0.0;
    double compScHigh     = 1.0;

    // 3. Protector de Frecuencias Ambientales
    bool enableAmbienceGuard = true;
    int    ambDistant     = 1;
    double ambTarget      = 0.388889;
    double ambRelease     = 0.0;
    double ambDetRel      = 0.458333;
    double ambScLow       = 0.2269;
    double ambScHigh      = 0.5293;
    int    ambBandMod     = 1;

    // 4. Ganancia previa (Headroom anti-clipping) y Espacialización HRIR
    double preampDb       = -6.0;
    bool   enableGraphicEq = true;
    bool   enableHeSuVi   = true;
};

class ApoConfigInjector
{
public:
    static constexpr const char* kTacticalGraphicEq =
        "GraphicEQ: 20 -3; 25 -2; 31.5 -0.8; 40 0.5; 50 1.5; 63 2.2; 80 2; 100 1.5; "
        "125 1.2; 160 0.8; 200 0.2; 250 -0.6; 315 -1.4; 400 -1.8; 500 -1.5; 630 -1; "
        "800 -0.5; 1000 0; 1250 0.3; 1600 0.6; 2000 1; 2500 1.4; 3150 1.6; 4000 1.2; "
        "5000 0.8; 6300 2.2; 8000 1.8; 10000 0.7; 12500 0.2; 16000 -0.5; 20000 -1.2";

    /**
     * Ruta canónica por defecto de Equalizer APO en Windows.
     */
    static std::filesystem::path getDefaultApoConfigPath();

    /**
     * Construye en memoria el contenido exacto de config.txt con terminaciones CRLF.
     */
    static std::string buildConfigContent(const ApoChainConfig& config = {});

    /**
     * Escribe el archivo config.txt de forma atómica y segura frente a lecturas concurrentes
     * del motor de audio WASAPI de Equalizer APO.
     *
     * Estrategia:
     *  1. Crea archivo temporal adyacente con FlushFileBuffers.
     *  2. Reemplaza atómicamente con MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH).
     *  3. Si Equalizer APO mantiene bloqueado el permiso de borrado/renombrado pero comparte escritura,
     *     ejecuta escritura directa con vaciado al disco y verificación byte a byte.
     */
    static bool writeConfigAtomically(
        const std::filesystem::path& configFilePath,
        const ApoChainConfig& config = {},
        std::string* outError = nullptr);
};

} // namespace dash::audio
