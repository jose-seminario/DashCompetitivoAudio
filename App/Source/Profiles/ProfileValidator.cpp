#include "ProfileValidator.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace dash::profile
{

namespace
{
    std::string toLowerString(std::string_view sv)
    {
        std::string result(sv);
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    bool containsCaseInsensitive(const std::string& haystack, const std::string& needle)
    {
        auto it = std::search(
            haystack.begin(), haystack.end(),
            needle.begin(), needle.end(),
            [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
        );
        return it != haystack.end();
    }
} // namespace

void ProfileValidationResult::addError(const std::string& prop, const std::string& msg)
{
    valid = false;
    issues.push_back({ ValidationIssue::Severity::Error, prop, msg });
}

void ProfileValidationResult::addWarning(const std::string& prop, const std::string& msg)
{
    issues.push_back({ ValidationIssue::Severity::Warning, prop, msg });
}

std::string ProfileValidationResult::formatSummary() const
{
    if (issues.empty())
        return "Perfil válido sin problemas detectados.";

    std::ostringstream ss;
    ss << (valid ? "Validación exitosa con advertencias:\n" : "Fallo de validación:\n");
    for (const auto& issue : issues)
    {
        ss << " - [" << (issue.severity == ValidationIssue::Severity::Error ? "ERROR" : "WARN")
           << "] " << issue.property << ": " << issue.message << "\n";
    }
    return ss.str();
}

bool ProfileValidator::isSafeProfileId(const std::string& profileId) noexcept
{
    if (profileId.empty() || profileId.length() > kMaxIdLength)
        return false;

    // Solo se admiten caracteres alfanuméricos ASCII, '-' y '_'
    for (char c : profileId)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_')
            return false;
    }

    // Prohibir explícitamente nombres reservados de DOS/Windows
    const std::string lower = toLowerString(profileId);
    if (lower == "con" || lower == "prn" || lower == "aux" || lower == "nul" ||
        lower == "com1" || lower == "com2" || lower == "lpt1" || lower == "lpt2")
    {
        return false;
    }

    return true;
}

ProfileValidationResult ProfileValidator::validateRawJsonText(const std::string& rawJson)
{
    ProfileValidationResult result;

    // 1. Límite de tamaño de archivo (1 MB)
    if (rawJson.size() > kMaxJsonSizeBytes)
    {
        result.addError("size", "El archivo de perfil supera el límite máximo permitido de 1 MB.");
        return result;
    }

    if (rawJson.empty())
    {
        result.addError("content", "El contenido JSON está vacío.");
        return result;
    }

    // 2. Prohibición estricta de extensiones y comandos ejecutables
    static const std::vector<std::string> forbiddenTokens = {
        ".exe", ".dll", ".bat", ".cmd", ".ps1", ".vbs", ".msi",
        "powershell", "cmd.exe", "wscript", "cscript", "reg.exe",
        "system32", "<script", "exec(", "eval("
    };

    for (const auto& token : forbiddenTokens)
    {
        if (containsCaseInsensitive(rawJson, token))
        {
            result.addError("security", "Se detectó contenido ejecutable no permitido en el perfil: '" + token + "'");
        }
    }

    return result;
}

ProfileValidationResult ProfileValidator::validateProfile(const AudioProfile& profile)
{
    ProfileValidationResult result;

    // 1. Identificador de Perfil
    if (!isSafeProfileId(profile.profileId))
    {
        result.addError("profileId", "El ID del perfil contiene caracteres no permitidos o supera 64 caracteres. Debe ser un slug seguro (ej: 'warzone-competitive').");
    }

    // 2. Versión de Esquema
    if (profile.schemaVersion < 1)
    {
        result.addError("schemaVersion", "Versión de esquema inválida (< 1).");
    }
    else if (profile.schemaVersion > AudioProfile::kCurrentSchemaVersion)
    {
        result.addError("schemaVersion", "Versión de esquema futura no soportada directamente; requiere migración previa.");
    }

    // 3. Nombres y textos
    if (profile.name.empty() || profile.name.length() > kMaxNameLength)
    {
        result.addError("name", "El nombre del perfil no puede estar vacío ni superar 128 caracteres.");
    }

    if (profile.description.length() > kMaxTextLength)
    {
        result.addError("description", "La descripción supera la longitud máxima permitida.");
    }

    // 4. Parámetros de Audio
    if (profile.audio.sampleRate < 22050.0 || profile.audio.sampleRate > 192000.0)
    {
        result.addError("audio.sampleRate", "Frecuencia de muestreo fuera del rango admisible [22050 .. 192000 Hz].");
    }

    if (profile.dspParams.headroomDb < -36.0f || profile.dspParams.headroomDb > 0.0f)
    {
        result.addError("dsp.headroomDb", "Margen dinámico (headroom) fuera de rango [-36.0 .. 0.0 dB].");
    }

    // 5. Dinámica (Dynamics)
    const auto& dyn = profile.dspParams.dynamics;
    if (dyn.targetLufs < -70.0f || dyn.targetLufs > 0.0f)
    {
        result.addError("dsp.dynamics.targetLufs", "Target LUFS fuera del rango acústico [-70.0 .. 0.0 LUFS].");
    }
    if (dyn.ratio < 1.0f || dyn.ratio > 30.0f)
    {
        result.addError("dsp.dynamics.ratio", "Ratio de compresión fuera del rango [1.0 .. 30.0].");
    }
    if (dyn.attackMs < 0.1f || dyn.attackMs > 1000.0f)
    {
        result.addError("dsp.dynamics.attackMs", "Tiempo de ataque fuera de rango [0.1 .. 1000.0 ms].");
    }
    if (dyn.releaseMs < 5.0f || dyn.releaseMs > 5000.0f)
    {
        result.addError("dsp.dynamics.releaseMs", "Tiempo de relajación fuera de rango [5.0 .. 5000.0 ms].");
    }

    // 6. Transitorios (Transient)
    const auto& trans = profile.dspParams.transient;
    if (trans.centerReductionDb < 0.0f || trans.centerReductionDb > 24.0f)
    {
        result.addError("dsp.transient.centerReductionDb", "Atenuación de disparos fuera de rango [0.0 .. 24.0 dB].");
    }
    if (trans.crossoverFrequencyHz < 80.0f || trans.crossoverFrequencyHz > 1200.0f)
    {
        result.addError("dsp.transient.crossoverHz", "Crossover de transitorios fuera de rango [80.0 .. 1200.0 Hz].");
    }

    // 7. Ambiente (Ambience)
    const auto& amb = profile.dspParams.ambience;
    if (amb.reductionDb < 0.0f || amb.reductionDb > 24.0f)
    {
        result.addError("dsp.ambience.reductionDb", "Reducción ambiental fuera de rango [0.0 .. 24.0 dB].");
    }

    // 8. Ecualizador (EQ)
    const auto& eq = profile.dspParams.equalizer;
    if (eq.footstepsDb < -12.0f || eq.footstepsDb > 12.0f)
    {
        result.addError("dsp.equalizer.footstepsDb", "Control de pasos fuera de rango [-12.0 .. +12.0 dB].");
    }
    if (eq.bassDb < -12.0f || eq.bassDb > 12.0f)
    {
        result.addError("dsp.equalizer.bassDb", "Control de graves fuera de rango [-12.0 .. +12.0 dB].");
    }

    // 9. Ecualizador Paramétrico (si está activo)
    if (profile.dspParams.useParametricEq)
    {
        for (size_t i = 0; i < profile.dspParams.parametricBands.size(); ++i)
        {
            const auto& band = profile.dspParams.parametricBands[i];
            if (band.enabled)
            {
                if (band.frequencyHz < 20.0f || band.frequencyHz > 20000.0f)
                {
                    result.addError("dsp.parametricBands[" + std::to_string(i) + "].frequencyHz",
                                    "Frecuencia fuera del espectro audible [20 .. 20000 Hz].");
                }
                if (band.gainDb < -24.0f || band.gainDb > 24.0f)
                {
                    result.addError("dsp.parametricBands[" + std::to_string(i) + "].gainDb",
                                    "Ganancia de banda fuera de rango [-24.0 .. +24.0 dB].");
                }
                if (band.q < 0.1f || band.q > 18.0f)
                {
                    result.addError("dsp.parametricBands[" + std::to_string(i) + "].q",
                                    "Factor Q fuera de rango [0.1 .. 18.0].");
                }
            }
        }
    }

    // 10. Limitador (Limiter)
    const auto& lim = profile.dspParams.limiter;
    if (lim.ceilingDb < -12.0f || lim.ceilingDb > 0.0f)
    {
        result.addError("dsp.limiter.ceilingDb", "Techo de limitador fuera de rango [-12.0 .. 0.0 dB].");
    }

    // 11. Espacial (Spatial)
    if (profile.enableSpatial)
    {
        const auto& spat = profile.spatialParams;
        if (spat.soundstageWidth < 0.2f || spat.soundstageWidth > 3.0f)
        {
            result.addError("spatial.soundstageWidth", "Anchura de escenario fuera de rango [0.2 .. 3.0].");
        }
        if (spat.apparentDistanceM < 0.2f || spat.apparentDistanceM > 10.0f)
        {
            result.addError("spatial.apparentDistanceM", "Distancia acústica aparente fuera de rango [0.2 .. 10.0 m].");
        }
        if (spat.frontBackSeparation < 0.0f || spat.frontBackSeparation > 2.0f)
        {
            result.addError("spatial.frontBackSeparation", "Separación frente/detrás fuera de rango [0.0 .. 2.0].");
        }
    }

    return result;
}

} // namespace dash::profile
