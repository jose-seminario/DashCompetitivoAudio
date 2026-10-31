#include "ProfileValidator.h"

#include <juce_core/juce_core.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <sstream>

namespace dash::profile
{

namespace
{
    template <typename T>
    inline bool isSafeNumeric(T val, T min, T max) noexcept
    {
        return std::isfinite(val) && val >= min && val <= max;
    }

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

bool ProfileValidator::isSafeProfileId(std::string_view profileId) noexcept
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

    // 2. Control de profundidad para prevenir Stack Overflow por recursión excesiva
    int currentDepth = 0;
    int maxDepth = 0;
    bool inString = false;
    bool escapeNext = false;

    for (char c : rawJson)
    {
        if (escapeNext)
        {
            escapeNext = false;
            continue;
        }
        if (c == '\\' && inString)
        {
            escapeNext = true;
            continue;
        }
        if (c == '"')
        {
            inString = !inString;
            continue;
        }
        if (!inString)
        {
            if (c == '{' || c == '[')
            {
                ++currentDepth;
                if (currentDepth > maxDepth)
                    maxDepth = currentDepth;
            }
            else if (c == '}' || c == ']')
            {
                --currentDepth;
            }
        }
    }

    constexpr int kMaxAllowedNestingDepth = 16;
    if (maxDepth > kMaxAllowedNestingDepth)
    {
        result.addError("security", "Profundidad de anidamiento JSON excesiva (supera el límite de seguridad de 16 niveles).");
        return result;
    }

    // 3. Parseo estructural mediante JUCE
    juce::var parsed;
    const auto parseRes = juce::JSON::parse(juce::String::fromUTF8(rawJson.data(), static_cast<int>(rawJson.size())), parsed);
    if (parseRes.failed())
    {
        result.addError("syntax", "Sintaxis JSON malformada: " + parseRes.getErrorMessage().toStdString());
        return result;
    }

    // 4. Validación estructural de claves prohibidas y ejecutables
    static const std::vector<std::string> forbiddenKeys = {
        "cmd", "command", "exec", "script", "library", "powershell", "shell",
        "run", "eval", "process", "system", "wscript", "cscript"
    };

    static const std::vector<std::string> executableExtensions = {
        ".exe", ".dll", ".bat", ".cmd", ".ps1", ".vbs", ".msi", ".com"
    };

    std::function<void(const juce::var&)> inspectVar = [&](const juce::var& v) {
        if (v.isObject())
        {
            if (auto* obj = v.getDynamicObject())
            {
                for (const auto& prop : obj->getProperties())
                {
                    const std::string keyLower = toLowerString(prop.name.toString().toStdString());
                    for (const auto& fk : forbiddenKeys)
                    {
                        if (keyLower == fk)
                        {
                            result.addError("security", "Clave no permitida detectada en el perfil: '" + keyLower + "'");
                            return;
                        }
                    }

                    if (prop.value.isString())
                    {
                        const std::string valLower = toLowerString(prop.value.toString().toStdString());
                        if (keyLower != "description" && keyLower != "name" && keyLower != "changelog" && keyLower != "variant")
                        {
                            for (const auto& ext : executableExtensions)
                            {
                                if (valLower.size() >= ext.size() &&
                                    valLower.compare(valLower.size() - ext.size(), ext.size(), ext) == 0)
                                {
                                    result.addError("security", "Se detectó extensión ejecutable no permitida en propiedad '" + keyLower + "': " + valLower);
                                    return;
                                }
                            }
                        }
                    }

                    inspectVar(prop.value);
                }
            }
        }
        else if (v.isArray())
        {
            if (auto* arr = v.getArray())
            {
                for (const auto& item : *arr)
                {
                    inspectVar(item);
                }
            }
        }
    };

    inspectVar(parsed);
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
    if (!isSafeNumeric(profile.audio.sampleRate, 22050.0, 192000.0))
    {
        result.addError("audio.sampleRate", "Frecuencia de muestreo fuera del rango admisible [22050 .. 192000 Hz].");
    }

    if (!isSafeNumeric(profile.dspParams.headroomDb, -36.0f, 0.0f))
    {
        result.addError("dsp.headroomDb", "Margen dinámico (headroom) fuera de rango [-36.0 .. 0.0 dB].");
    }

    // 5. Dinámica (Dynamics)
    const auto& dyn = profile.dspParams.dynamics;
    if (!isSafeNumeric(dyn.targetLufs, -70.0f, 0.0f))
    {
        result.addError("dsp.dynamics.targetLufs", "Target LUFS fuera del rango acústico [-70.0 .. 0.0 LUFS].");
    }
    if (!isSafeNumeric(dyn.ratio, 1.0f, 30.0f))
    {
        result.addError("dsp.dynamics.ratio", "Ratio de compresión fuera del rango [1.0 .. 30.0].");
    }
    if (!isSafeNumeric(dyn.attackMs, 0.1f, 1000.0f))
    {
        result.addError("dsp.dynamics.attackMs", "Tiempo de ataque fuera de rango [0.1 .. 1000.0 ms].");
    }
    if (!isSafeNumeric(dyn.releaseMs, 5.0f, 5000.0f))
    {
        result.addError("dsp.dynamics.releaseMs", "Tiempo de relajación fuera de rango [5.0 .. 5000.0 ms].");
    }

    // 6. Transitorios (Transient)
    const auto& trans = profile.dspParams.transient;
    if (!isSafeNumeric(trans.gun, 0.0f, 1.0f))
    {
        result.addError("dsp.transient.gun", "Ganancia objetivo de disparos (gun) fuera de rango [0.0 .. 1.0].");
    }
    if (!isSafeNumeric(trans.frequencySplitHz, dash::core::TransientProcessor::kMinSplitHz, dash::core::TransientProcessor::kMaxSplitHz))
    {
        result.addError("dsp.transient.frequencySplitHz", "Crossover de transitorios fuera de rango [150.0 .. 1200.0 Hz].");
    }

    // 7. Ambiente (Ambience)
    const auto& amb = profile.dspParams.ambience;
    if (!isSafeNumeric(amb.ambGuar, 0.0f, 1.0f))
    {
        result.addError("dsp.ambience.ambGuar", "Factor de reducción ambiental (ambGuar) fuera de rango [0.0 .. 1.0].");
    }
    if (!isSafeNumeric(amb.scLow, 0.0f, 1.0f))
    {
        result.addError("dsp.ambience.scLow", "Corte inferior normalizado (scLow) fuera de rango [0.0 .. 1.0].");
    }
    if (!isSafeNumeric(amb.scHigh, 0.0f, 1.0f))
    {
        result.addError("dsp.ambience.scHigh", "Corte superior normalizado (scHigh) fuera de rango [0.0 .. 1.0].");
    }

    // 8. Ecualizador (EQ)
    const auto& eq = profile.dspParams.equalizer;
    if (!isSafeNumeric(eq.footstepsDb, -12.0f, 12.0f))
    {
        result.addError("dsp.equalizer.footstepsDb", "Control de pasos fuera de rango [-12.0 .. +12.0 dB].");
    }
    if (!isSafeNumeric(eq.bassDb, -12.0f, 12.0f))
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
                if (!isSafeNumeric(band.frequencyHz, 20.0f, 20000.0f))
                {
                    result.addError("dsp.parametricBands[" + std::to_string(i) + "].frequencyHz",
                                    "Frecuencia fuera del espectro audible [20 .. 20000 Hz].");
                }
                if (!isSafeNumeric(band.gainDb, -24.0f, 24.0f))
                {
                    result.addError("dsp.parametricBands[" + std::to_string(i) + "].gainDb",
                                    "Ganancia de banda fuera de rango [-24.0 .. +24.0 dB].");
                }
                if (!isSafeNumeric(band.q, 0.1f, 18.0f))
                {
                    result.addError("dsp.parametricBands[" + std::to_string(i) + "].q",
                                    "Factor Q fuera de rango [0.1 .. 18.0].");
                }
            }
        }
    }

    // 10. Limitador (Limiter)
    const auto& lim = profile.dspParams.limiter;
    if (!isSafeNumeric(lim.ceilingDb, -12.0f, 0.0f))
    {
        result.addError("dsp.limiter.ceilingDb", "Techo de limitador fuera de rango [-12.0 .. 0.0 dB].");
    }

    // 11. Espacial (Spatial)
    if (profile.enableSpatial)
    {
        const auto& spat = profile.spatialParams;
        if (!isSafeNumeric(spat.soundstageWidth, 0.2f, 3.0f))
        {
            result.addError("spatial.soundstageWidth", "Anchura de escenario fuera de rango [0.2 .. 3.0].");
        }
        if (!isSafeNumeric(spat.apparentDistanceM, 0.2f, 10.0f))
        {
            result.addError("spatial.apparentDistanceM", "Distancia acústica aparente fuera de rango [0.2 .. 10.0 m].");
        }
        if (!isSafeNumeric(spat.frontBackSeparation, 0.0f, 2.0f))
        {
            result.addError("spatial.frontBackSeparation", "Separación frente/detrás fuera de rango [0.0 .. 2.0].");
        }
    }

    return result;
}

} // namespace dash::profile
