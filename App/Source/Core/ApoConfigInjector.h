#pragma once

#include <filesystem>
#include <string>

namespace dash::audio
{

struct ApoChainConfig
{
    std::string profileName = "Campaign";

    // Filtro opcional de dispositivo Equalizer APO, ej. "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
    std::string targetEndpointGuid;

    // 1. Supresor de Transitorios (Disparos - Gun Crack)
    bool   enableTransientSuppressor = true;
    double gunVolume                 = 0.450000;
    int    gunSteps                  = 0;
    int    gunFrontOnly              = 1;
    int    gunRestore                = 0;
    int    gunBypass                 = 0;

    // 2. Compresor Dinámico Híbrido de Sonoridad (ITU-R BS.1770)
    bool   enableLoudnessCompressor = true;
    double compAmount               = 0.750000;
    double compTarget               = 0.277778; // Equivale a -18.0 LUFS en escala normalizada [-28..+8]
    double compRelease              = 0.5;
    double compDetRel               = 0.583333;
    double compLimRel               = 0.612903;
    double compScLow                = 0.0;
    double compScHigh               = 1.0;

    // 3. Protector de Frecuencias Ambientales (Multibanda IIR 350 Hz - 2.5 kHz)
    bool   enableAmbienceGuard = true;
    double ambGuard            = 1.0;      // Parámetro AmbGuar [0.0 .. 1.0]
    int    ambDistant          = 1;
    double ambTarget           = 0.388889;
    double ambRelease          = 0.0;
    double ambDetRel           = 0.458333;
    double ambScLow            = 0.2269;
    double ambScHigh           = 0.5293;
    int    ambBandMod          = 1;

    // 4. Ganancia previa, Personalización Extrema de EQ (Pasos / Graves) y HeSuVi
    double preampDb        = -6.0;
    bool   enableGraphicEq = true;
    double footstepsEqDb   = 1.6; // Por defecto +1.6 dB en 3.15 kHz y +2.2 dB en 6.3 kHz (rango -3 a +6 dB)
    double bassEqDb        = 1.5; // Por defecto +1.5 dB en 100 Hz / +2.0 dB en 80 Hz (banda 100-250 Hz)
    bool   enableHeSuVi    = true;
    std::string hesuviIncludePath; // Vacío = resolución dinámica (estándar: "HeSuVi\\hesuvi.txt")
};

class ApoConfigInjector
{
public:
    static constexpr const char* kDefaultHeSuViInclude = "HeSuVi\\hesuvi.txt";

    static constexpr const char* kTacticalGraphicEq =
        "GraphicEQ: 20 -3; 25 -2; 31.5 -0.8; 40 0.5; 50 1.5; 63 2.2; 80 2; 100 1.5; "
        "125 1.2; 160 0.8; 200 0.2; 250 -0.6; 315 -1.4; 400 -1.8; 500 -1.5; 630 -1; "
        "800 -0.5; 1000 0; 1250 0.3; 1600 0.6; 2000 1; 2500 1.4; 3150 1.6; 4000 1.2; "
        "5000 0.8; 6300 2.2; 8000 1.8; 10000 0.7; 12500 0.2; 16000 -0.5; 20000 -1.2";

    /**
     * Ruta dinámica de config.txt en Equalizer APO (consulta HKLM\SOFTWARE\EqualizerAPO o %ProgramFiles%).
     */
    static std::filesystem::path getDefaultApoConfigPath();

    /**
     * Resuelve dinámicamente la ruta de inclusión de HeSuVi respecto a config.txt.
     * Cuando HeSuVi reside en <EqualizerAPO>\config\HeSuVi\hesuvi.txt devuelve la ruta
     * relativa estándar "HeSuVi\hesuvi.txt"; si reside en una ruta externa alternativa,
     * devuelve su ruta resuelta.
     */
    static std::string resolveHeSuViIncludePath(
        const std::filesystem::path& configFilePath,
        const std::string& customOverride = {});

    /**
     * Genera la directiva GraphicEQ de 31 bandas aplicando las perillas de Pasos (3.15/6.3 kHz)
     * y Graves (100-250 Hz) sobre la curva base de nuestro motor de optimización táctica.
     */
    static std::string buildGraphicEqDirective(double footstepsDb = 1.6, double bassDb = 1.5);

    /**
     * Construye en memoria el contenido exacto de config.txt con terminaciones CRLF.
     */
    static std::string buildConfigContent(
        const ApoChainConfig& config = {},
        const std::filesystem::path& targetConfigFilePath = {});

    /**
     * Escribe el archivo config.txt de forma atómica y segura frente a lecturas concurrentes
     * del motor de audio WASAPI de Equalizer APO.
     */
    static bool writeConfigAtomically(
        const std::filesystem::path& configFilePath,
        const ApoChainConfig& config = {},
        std::string* outError = nullptr,
        bool* outPermissionDenied = nullptr);

    /**
     * Escribe un config.txt vacío de forma atómica para desactivar por completo el DSP
     * al restaurar el sistema de audio original.
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
