#include "ProfileSerializer.h"
#include "ProfileValidator.h"
#include "ProfileMigrator.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace dash::profile
{

namespace
{
    const char* filterTypeToString(dash::core::ParametricFilterType type) noexcept
    {
        switch (type)
        {
            case dash::core::ParametricFilterType::Peaking:   return "peaking";
            case dash::core::ParametricFilterType::LowShelf:  return "low_shelf";
            case dash::core::ParametricFilterType::HighShelf: return "high_shelf";
            case dash::core::ParametricFilterType::HighPass:  return "high_pass";
            case dash::core::ParametricFilterType::LowPass:   return "low_pass";
            case dash::core::ParametricFilterType::Notch:     return "notch";
            default:                                          return "peaking";
        }
    }

    dash::core::ParametricFilterType stringToFilterType(const juce::String& str) noexcept
    {
        const auto lower = str.toLowerCase().trim();
        if (lower == "low_shelf" || lower == "lowshelf")   return dash::core::ParametricFilterType::LowShelf;
        if (lower == "high_shelf" || lower == "highshelf") return dash::core::ParametricFilterType::HighShelf;
        if (lower == "high_pass" || lower == "highpass")   return dash::core::ParametricFilterType::HighPass;
        if (lower == "low_pass" || lower == "lowpass")     return dash::core::ParametricFilterType::LowPass;
        if (lower == "notch")                              return dash::core::ParametricFilterType::Notch;
        return dash::core::ParametricFilterType::Peaking;
    }
} // namespace

std::string ProfileSerializer::serializeToJson(const AudioProfile& profile, bool prettyPrint)
{
    auto* rootObj = new juce::DynamicObject();
    juce::var root(rootObj);

    rootObj->setProperty("schemaVersion", profile.schemaVersion);
    rootObj->setProperty("profileId",     juce::String(profile.profileId));
    rootObj->setProperty("name",          juce::String(profile.name));
    rootObj->setProperty("description",   juce::String(profile.description));

    // Game
    auto* gameObj = new juce::DynamicObject();
    gameObj->setProperty("id",               juce::String(profile.game.id));
    gameObj->setProperty("name",             juce::String(profile.game.name));
    gameObj->setProperty("supportedVersion", juce::String(profile.game.supportedVersion));
    gameObj->setProperty("tacticalMode",     juce::String(profile.game.tacticalMode));
    rootObj->setProperty("game", juce::var(gameObj));

    // Headphone
    auto* hpObj = new juce::DynamicObject();
    hpObj->setProperty("manufacturer",      juce::String(profile.headphone.manufacturer));
    hpObj->setProperty("model",             juce::String(profile.headphone.model));
    hpObj->setProperty("variant",           juce::String(profile.headphone.variant));
    hpObj->setProperty("calibrationStatus", juce::String(profile.headphone.calibrationStatus));
    hpObj->setProperty("targetCurve",       juce::String(profile.headphone.targetCurve));
    hpObj->setProperty("measurementSource", juce::String(profile.headphone.measurementSource));
    hpObj->setProperty("measurementDate",   juce::String(profile.headphone.measurementDate));
    rootObj->setProperty("headphone", juce::var(hpObj));

    // Audio Settings
    auto* audioObj = new juce::DynamicObject();
    audioObj->setProperty("sampleRate",    profile.audio.sampleRate);
    audioObj->setProperty("channelLayout", juce::String(profile.audio.channelLayout));
    audioObj->setProperty("preampDb",      profile.audio.preampDb);
    rootObj->setProperty("audio", juce::var(audioObj));

    // Processing
    auto* procObj = new juce::DynamicObject();
    procObj->setProperty("headroomDb", profile.dspParams.headroomDb);
    procObj->setProperty("bypassAll",  profile.dspParams.bypassAll);

    // Dynamics
    auto* dynObj = new juce::DynamicObject();
    dynObj->setProperty("targetLufs",     profile.dspParams.dynamics.targetLufs);
    dynObj->setProperty("ratio",          profile.dspParams.dynamics.ratio);
    dynObj->setProperty("attackMs",       profile.dspParams.dynamics.attackMs);
    dynObj->setProperty("releaseMs",      profile.dspParams.dynamics.releaseMs);
    dynObj->setProperty("upwardAmount",   profile.dspParams.dynamics.upwardAmount);
    dynObj->setProperty("downwardAmount", profile.dspParams.dynamics.downwardAmount);
    dynObj->setProperty("makeupGainDb",   profile.dspParams.dynamics.makeupGainDb);
    dynObj->setProperty("bypass",         profile.dspParams.dynamics.bypass);
    procObj->setProperty("dynamics", juce::var(dynObj));

    // Transient
    auto* transObj = new juce::DynamicObject();
    transObj->setProperty("gun",              profile.dspParams.transient.gun);
    transObj->setProperty("frequencySplitHz", profile.dspParams.transient.frequencySplitHz);
    transObj->setProperty("attackMs",         profile.dspParams.transient.attackMs);
    transObj->setProperty("releaseMs",        profile.dspParams.transient.releaseMs);
    transObj->setProperty("bypass",           profile.dspParams.transient.bypass);
    procObj->setProperty("transient", juce::var(transObj));

    // Ambience
    auto* ambObj = new juce::DynamicObject();
    ambObj->setProperty("ambGuar", profile.dspParams.ambience.ambGuar);
    ambObj->setProperty("scLow",   profile.dspParams.ambience.scLow);
    ambObj->setProperty("scHigh",  profile.dspParams.ambience.scHigh);
    ambObj->setProperty("bypass",  profile.dspParams.ambience.bypass);
    procObj->setProperty("ambience", juce::var(ambObj));

    // Equalizer (31 bandas)
    auto* eqObj = new juce::DynamicObject();
    eqObj->setProperty("footstepsDb", profile.dspParams.equalizer.footstepsDb);
    eqObj->setProperty("bassDb",      profile.dspParams.equalizer.bassDb);
    eqObj->setProperty("preampDb",    profile.dspParams.equalizer.preampDb);
    eqObj->setProperty("bypass",      profile.dspParams.equalizer.bypass);
    procObj->setProperty("equalizer", juce::var(eqObj));

    // Parametric EQ
    auto* peqObj = new juce::DynamicObject();
    peqObj->setProperty("enabled", profile.dspParams.useParametricEq);

    juce::Array<juce::var> bandsArray;
    for (size_t i = 0; i < profile.dspParams.parametricBands.size(); ++i)
    {
        const auto& band = profile.dspParams.parametricBands[i];
        auto* bObj = new juce::DynamicObject();
        bObj->setProperty("index",       static_cast<int>(i));
        bObj->setProperty("type",        juce::String(filterTypeToString(band.type)));
        bObj->setProperty("frequencyHz", band.frequencyHz);
        bObj->setProperty("gainDb",      band.gainDb);
        bObj->setProperty("q",           band.q);
        bObj->setProperty("enabled",     band.enabled);
        bandsArray.add(juce::var(bObj));
    }
    peqObj->setProperty("bands", juce::var(bandsArray));
    procObj->setProperty("parametricEq", juce::var(peqObj));

    // Limiter
    auto* limObj = new juce::DynamicObject();
    limObj->setProperty("ceilingDb", profile.dspParams.limiter.ceilingDb);
    limObj->setProperty("releaseMs", profile.dspParams.limiter.releaseMs);
    limObj->setProperty("bypass",    profile.dspParams.limiter.bypass);
    procObj->setProperty("limiter", juce::var(limObj));

    rootObj->setProperty("processing", juce::var(procObj));

    // Spatial
    auto* spatObj = new juce::DynamicObject();
    spatObj->setProperty("enabled",             profile.enableSpatial);
    spatObj->setProperty("soundstageWidth",     profile.spatialParams.soundstageWidth);
    spatObj->setProperty("apparentDistanceM",   profile.spatialParams.apparentDistanceM);
    spatObj->setProperty("frontBackSeparation", profile.spatialParams.frontBackSeparation);
    spatObj->setProperty("elevationEmphasis",   profile.spatialParams.elevationEmphasis);
    spatObj->setProperty("bypass",              profile.spatialParams.bypass);
    rootObj->setProperty("spatial", juce::var(spatObj));

    // Metadata
    auto* metaObj = new juce::DynamicObject();
    metaObj->setProperty("author",    juce::String(profile.metadata.author));
    metaObj->setProperty("license",   juce::String(profile.metadata.license));
    metaObj->setProperty("version",   juce::String(profile.metadata.version));
    metaObj->setProperty("createdAt", juce::String(profile.metadata.createdAt));
    metaObj->setProperty("changelog", juce::String(profile.metadata.changelog));
    metaObj->setProperty("verified",  profile.metadata.verified);

    juce::Array<juce::var> tagsArray;
    for (const auto& tag : profile.metadata.tags)
        tagsArray.add(juce::var(juce::String(tag)));
    metaObj->setProperty("tags", juce::var(tagsArray));

    rootObj->setProperty("metadata", juce::var(metaObj));

    const juce::String formatted = juce::JSON::toString(root, !prettyPrint);
    return formatted.toStdString();
}

bool ProfileSerializer::deserializeFromJson(const std::string& jsonText, AudioProfile& outProfile, std::string* outError)
{
    // 1. Auditoría de seguridad del texto crudo (límite de tamaño y tokens maliciosos)
    const auto preValidation = ProfileValidator::validateRawJsonText(jsonText);
    if (preValidation.hasErrors())
    {
        if (outError)
            *outError = preValidation.formatSummary();
        return false;
    }

    std::string textToParse = jsonText;

    // Detectar si el esquema requiere migración previa
    juce::var probeRoot;
    if (juce::JSON::parse(juce::String::fromUTF8(jsonText.data(), static_cast<int>(jsonText.size())), probeRoot).wasOk() && probeRoot.isObject())
    {
        const int schemaVer = static_cast<int>(probeRoot.getProperty("schemaVersion", 0));
        if (schemaVer < AudioProfile::kCurrentSchemaVersion && ProfileMigrator::canMigrate(schemaVer))
        {
            std::string migratedJson;
            std::string migErr;
            if (ProfileMigrator::migrateJson(jsonText, migratedJson, &migErr))
            {
                textToParse = std::move(migratedJson);
            }
        }
    }

    // 2. Parseo mediante motor nativo JUCE
    juce::var root;
    const auto parseResult = juce::JSON::parse(juce::String::fromUTF8(textToParse.data(), static_cast<int>(textToParse.size())), root);
    if (parseResult.failed())
    {
        if (outError)
            *outError = "Error de sintaxis JSON: " + parseResult.getErrorMessage().toStdString();
        return false;
    }

    if (!root.isObject())
    {
        if (outError)
            *outError = "La raíz del JSON debe ser un objeto.";
        return false;
    }

    // 3. Mapeo estructurado
    AudioProfile p;
    p.resetToDefaults();

    p.schemaVersion = static_cast<int>(root.getProperty("schemaVersion", 1));
    p.profileId     = root.getProperty("profileId", "default").toString().toStdString();
    p.name          = root.getProperty("name", "Unnamed Profile").toString().toStdString();
    p.description   = root.getProperty("description", "").toString().toStdString();

    // Game
    if (root.hasProperty("game"))
    {
        auto g = root.getProperty("game", juce::var());
        p.game.id               = g.getProperty("id", "call-of-duty-warzone").toString().toStdString();
        p.game.name             = g.getProperty("name", "Call of Duty: Warzone").toString().toStdString();
        p.game.supportedVersion = g.getProperty("supportedVersion", "MW3 / Warzone 2.0+").toString().toStdString();
        p.game.tacticalMode     = g.getProperty("tacticalMode", "Footsteps Focus").toString().toStdString();
    }

    // Headphone
    if (root.hasProperty("headphone"))
    {
        auto h = root.getProperty("headphone", juce::var());
        p.headphone.manufacturer      = h.getProperty("manufacturer", "Generic").toString().toStdString();
        p.headphone.model             = h.getProperty("model", "Headphones").toString().toStdString();
        p.headphone.variant           = h.getProperty("variant", "Default").toString().toStdString();
        p.headphone.calibrationStatus = h.getProperty("calibrationStatus", "uncalibrated").toString().toStdString();
        p.headphone.targetCurve       = h.getProperty("targetCurve", "Harman Over-Ear 2018").toString().toStdString();
        p.headphone.measurementSource = h.getProperty("measurementSource", "None").toString().toStdString();
        p.headphone.measurementDate   = h.getProperty("measurementDate", "").toString().toStdString();
    }

    // Audio
    if (root.hasProperty("audio"))
    {
        auto a = root.getProperty("audio", juce::var());
        p.audio.sampleRate    = static_cast<double>(a.getProperty("sampleRate", 48000.0));
        p.audio.channelLayout = a.getProperty("channelLayout", "7.1").toString().toStdString();
        p.audio.preampDb      = static_cast<float>(a.getProperty("preampDb", -6.0));
    }

    // Processing
    if (root.hasProperty("processing"))
    {
        auto proc = root.getProperty("processing", juce::var());
        p.dspParams.headroomDb = static_cast<float>(proc.getProperty("headroomDb", -6.0));
        p.dspParams.bypassAll  = static_cast<bool>(proc.getProperty("bypassAll", false));

        // Dynamics
        if (proc.hasProperty("dynamics"))
        {
            auto d = proc.getProperty("dynamics", juce::var());
            p.dspParams.dynamics.targetLufs     = static_cast<float>(d.getProperty("targetLufs", -16.0));
            p.dspParams.dynamics.ratio          = static_cast<float>(d.getProperty("ratio", 2.5));
            p.dspParams.dynamics.attackMs       = static_cast<float>(d.getProperty("attackMs", 20.0));
            p.dspParams.dynamics.releaseMs      = static_cast<float>(d.getProperty("releaseMs", 120.0));
            p.dspParams.dynamics.upwardAmount   = static_cast<float>(d.getProperty("upwardAmount", 0.5));
            p.dspParams.dynamics.downwardAmount = static_cast<float>(d.getProperty("downwardAmount", 0.5));
            p.dspParams.dynamics.makeupGainDb   = static_cast<float>(d.getProperty("makeupGainDb", 6.0));
            p.dspParams.dynamics.bypass         = static_cast<bool>(d.getProperty("bypass", false));
        }

        // Transient
        if (proc.hasProperty("transient"))
        {
            auto t = proc.getProperty("transient", juce::var());
            if (t.hasProperty("gun"))
            {
                p.dspParams.transient.gun = std::clamp(static_cast<float>(t.getProperty("gun", 0.45)), 0.0f, 1.0f);
            }
            else if (t.hasProperty("centerReductionDb"))
            {
                const float redDb = std::clamp(static_cast<float>(t.getProperty("centerReductionDb", 4.0)), 0.0f, 24.0f);
                p.dspParams.transient.gun = std::clamp(std::pow(10.0f, -redDb / 20.0f), 0.0f, 1.0f);
            }

            if (t.hasProperty("frequencySplitHz"))
            {
                p.dspParams.transient.frequencySplitHz = std::clamp(
                    static_cast<float>(t.getProperty("frequencySplitHz", 280.0)),
                    dash::core::TransientProcessor::kMinSplitHz,
                    dash::core::TransientProcessor::kMaxSplitHz);
            }
            else if (t.hasProperty("crossoverFrequencyHz"))
            {
                p.dspParams.transient.frequencySplitHz = std::clamp(
                    static_cast<float>(t.getProperty("crossoverFrequencyHz", 280.0)),
                    dash::core::TransientProcessor::kMinSplitHz,
                    dash::core::TransientProcessor::kMaxSplitHz);
            }

            p.dspParams.transient.attackMs  = static_cast<float>(t.getProperty("attackMs", 0.2));
            p.dspParams.transient.releaseMs = static_cast<float>(t.getProperty("releaseMs", 45.0));
            p.dspParams.transient.bypass    = static_cast<bool>(t.getProperty("bypass", false));
        }

        // Ambience
        if (proc.hasProperty("ambience"))
        {
            auto amb = proc.getProperty("ambience", juce::var());
            if (amb.hasProperty("ambGuar"))
            {
                p.dspParams.ambience.ambGuar = std::clamp(static_cast<float>(amb.getProperty("ambGuar", 1.0)), 0.0f, 1.0f);
            }
            else if (amb.hasProperty("reductionDb"))
            {
                const float redDb = std::max(0.0f, static_cast<float>(amb.getProperty("reductionDb", 6.0)));
                p.dspParams.ambience.ambGuar = std::clamp(redDb / 6.0f, 0.0f, 1.0f);
            }

            if (amb.hasProperty("scLow"))
            {
                p.dspParams.ambience.scLow = std::clamp(static_cast<float>(amb.getProperty("scLow", 0.2269)), 0.0f, 1.0f);
            }
            else if (amb.hasProperty("lowCutoffHz"))
            {
                const float hz = std::clamp(static_cast<float>(amb.getProperty("lowCutoffHz", 350.0)), 20.0f, 20000.0f);
                p.dspParams.ambience.scLow = std::clamp(std::log10(hz / 20.0f) / 3.0f, 0.0f, 1.0f);
            }

            if (amb.hasProperty("scHigh"))
            {
                p.dspParams.ambience.scHigh = std::clamp(static_cast<float>(amb.getProperty("scHigh", 0.5293)), 0.0f, 1.0f);
            }
            else if (amb.hasProperty("highCutoffHz"))
            {
                const float hz = std::clamp(static_cast<float>(amb.getProperty("highCutoffHz", 2500.0)), 20.0f, 20000.0f);
                p.dspParams.ambience.scHigh = std::clamp(std::log10(hz / 20.0f) / 3.0f, 0.0f, 1.0f);
            }

            p.dspParams.ambience.bypass = static_cast<bool>(amb.getProperty("bypass", false));
        }

        // Equalizer
        if (proc.hasProperty("equalizer"))
        {
            auto eq = proc.getProperty("equalizer", juce::var());
            p.dspParams.equalizer.footstepsDb = static_cast<float>(eq.getProperty("footstepsDb", 1.6));
            p.dspParams.equalizer.bassDb      = static_cast<float>(eq.getProperty("bassDb", 1.5));
            p.dspParams.equalizer.preampDb    = static_cast<float>(eq.getProperty("preampDb", 0.0));
            p.dspParams.equalizer.bypass      = static_cast<bool>(eq.getProperty("bypass", false));
        }

        // Parametric EQ
        if (proc.hasProperty("parametricEq"))
        {
            auto peq = proc.getProperty("parametricEq", juce::var());
            p.dspParams.useParametricEq = static_cast<bool>(peq.getProperty("enabled", false));

            if (peq.hasProperty("bands"))
            {
                auto bands = peq.getProperty("bands", juce::var());
                if (bands.isArray())
                {
                    auto* arr = bands.getArray();
                    for (int i = 0; i < arr->size() && i < static_cast<int>(p.dspParams.parametricBands.size()); ++i)
                    {
                        const auto& b = (*arr)[i];
                        auto& targetBand = p.dspParams.parametricBands[static_cast<size_t>(i)];
                        targetBand.type        = stringToFilterType(b.getProperty("type", "peaking").toString());
                        targetBand.frequencyHz = static_cast<float>(b.getProperty("frequencyHz", 1000.0));
                        targetBand.gainDb      = static_cast<float>(b.getProperty("gainDb", 0.0));
                        targetBand.q           = static_cast<float>(b.getProperty("q", 0.7071));
                        targetBand.enabled     = static_cast<bool>(b.getProperty("enabled", true));
                    }
                }
            }
        }

        // Limiter
        if (proc.hasProperty("limiter"))
        {
            auto lim = proc.getProperty("limiter", juce::var());
            p.dspParams.limiter.ceilingDb = static_cast<float>(lim.getProperty("ceilingDb", -0.5));
            p.dspParams.limiter.releaseMs = static_cast<float>(lim.getProperty("releaseMs", 50.0));
            p.dspParams.limiter.bypass    = static_cast<bool>(lim.getProperty("bypass", false));
        }
    }

    // Spatial
    if (root.hasProperty("spatial"))
    {
        auto spat = root.getProperty("spatial", juce::var());
        p.enableSpatial                     = static_cast<bool>(spat.getProperty("enabled", false));
        p.spatialParams.soundstageWidth     = static_cast<float>(spat.getProperty("soundstageWidth", 1.0));
        p.spatialParams.apparentDistanceM   = static_cast<float>(spat.getProperty("apparentDistanceM", 1.4));
        p.spatialParams.frontBackSeparation = static_cast<float>(spat.getProperty("frontBackSeparation", 1.0));
        p.spatialParams.elevationEmphasis   = static_cast<float>(spat.getProperty("elevationEmphasis", 0.0));
        p.spatialParams.bypass              = static_cast<bool>(spat.getProperty("bypass", false));
    }

    // Metadata
    if (root.hasProperty("metadata"))
    {
        auto m = root.getProperty("metadata", juce::var());
        p.metadata.author    = m.getProperty("author", "DashAudio").toString().toStdString();
        p.metadata.license   = m.getProperty("license", "CC-BY-4.0").toString().toStdString();
        p.metadata.version   = m.getProperty("version", "1.0.0").toString().toStdString();
        p.metadata.createdAt = m.getProperty("createdAt", "").toString().toStdString();
        p.metadata.changelog = m.getProperty("changelog", "").toString().toStdString();
        p.metadata.verified  = static_cast<bool>(m.getProperty("verified", false));

        if (m.hasProperty("tags"))
        {
            auto tags = m.getProperty("tags", juce::var());
            if (tags.isArray())
            {
                p.metadata.tags.clear();
                for (int i = 0; i < tags.getArray()->size(); ++i)
                    p.metadata.tags.push_back((*tags.getArray())[i].toString().toStdString());
            }
        }
    }

    // 4. Validación final de coherencia y rangos en memoria
    const auto postValidation = ProfileValidator::validateProfile(p);
    if (postValidation.hasErrors())
    {
        if (outError)
            *outError = postValidation.formatSummary();
        return false;
    }

    outProfile = std::move(p);
    return true;
}

bool ProfileSerializer::saveToFile(const AudioProfile& profile, const std::filesystem::path& filePath, std::string* outError)
{
    try
    {
        // 1. Validar perfil antes de guardar
        const auto val = ProfileValidator::validateProfile(profile);
        if (val.hasErrors())
        {
            if (outError)
                *outError = val.formatSummary();
            return false;
        }

        // 2. Serializar a JSON
        const std::string jsonText = serializeToJson(profile, true);

        // 3. Crear directorio si no existe
        const auto parentDir = filePath.parent_path();
        if (!parentDir.empty())
            std::filesystem::create_directories(parentDir);

        // 4. Escritura atómica mediante archivo temporal
        const auto tempPath = filePath.string() + ".tmp";
        {
            std::ofstream out(tempPath, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!out.is_open())
            {
                if (outError)
                    *outError = "No se pudo abrir el archivo temporal para escritura: " + tempPath;
                return false;
            }
            out.write(jsonText.data(), static_cast<std::streamsize>(jsonText.size()));
            if (out.fail())
            {
                if (outError)
                    *outError = "Fallo de escritura en el archivo temporal.";
                return false;
            }
        }

        // Reemplazo atómico
        std::error_code ec;
        std::filesystem::rename(tempPath, filePath, ec);
        if (ec)
        {
            // Si rename falla por archivo existente en Windows, eliminar y mover
            std::filesystem::remove(filePath, ec);
            std::filesystem::rename(tempPath, filePath, ec);
            if (ec)
            {
                if (outError)
                    *outError = "Error al reemplazar archivo final: " + ec.message();
                return false;
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        if (outError)
            *outError = std::string("Excepción al guardar perfil: ") + e.what();
        return false;
    }
}

bool ProfileSerializer::loadFromFile(const std::filesystem::path& filePath, AudioProfile& outProfile, std::string* outError)
{
    try
    {
        if (!std::filesystem::exists(filePath))
        {
            if (outError)
                *outError = "El archivo no existe: " + filePath.string();
            return false;
        }

        const auto fileSize = std::filesystem::file_size(filePath);
        if (fileSize > ProfileValidator::kMaxJsonSizeBytes)
        {
            if (outError)
                *outError = "El archivo supera el tamaño máximo permitido de 1 MB.";
            return false;
        }

        std::ifstream in(filePath, std::ios::in | std::ios::binary);
        if (!in.is_open())
        {
            if (outError)
                *outError = "No se pudo abrir el archivo para lectura: " + filePath.string();
            return false;
        }

        std::string jsonText((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return deserializeFromJson(jsonText, outProfile, outError);
    }
    catch (const std::exception& e)
    {
        if (outError)
            *outError = std::string("Excepción al cargar perfil: ") + e.what();
        return false;
    }
}

} // namespace dash::profile
