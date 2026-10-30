#include "ProfileMigrator.h"
#include "ProfileSerializer.h"
#include "ProfileValidator.h"

#include <juce_core/juce_core.h>

namespace dash::profile
{

bool ProfileMigrator::canMigrate(int sourceVersion) noexcept
{
    // Soporta migración desde perfiles sin versión (0) y versión 1
    return sourceVersion >= 0 && sourceVersion <= AudioProfile::kCurrentSchemaVersion;
}

bool ProfileMigrator::migrateJson(const std::string& sourceJson, std::string& outMigratedJson, std::string* outError)
{
    // Validar seguridad del texto previo
    const auto val = ProfileValidator::validateRawJsonText(sourceJson);
    if (val.hasErrors())
    {
        if (outError)
            *outError = val.formatSummary();
        return false;
    }

    juce::var root;
    const auto parseResult = juce::JSON::parse(juce::String::fromUTF8(sourceJson.data(), static_cast<int>(sourceJson.size())), root);
    if (parseResult.failed() || !root.isObject())
    {
        if (outError)
            *outError = "JSON inválido para migración.";
        return false;
    }

    auto* obj = root.getDynamicObject();
    int version = static_cast<int>(obj->getProperty("schemaVersion"));

    // Migración de v0 (sin versión) a v1
    if (version == 0)
    {
        obj->setProperty("schemaVersion", 1);

        // Si no tiene sección game, inyectar valores seguros por defecto
        if (!obj->hasProperty("game"))
        {
            auto* gObj = new juce::DynamicObject();
            gObj->setProperty("id", "call-of-duty-warzone");
            gObj->setProperty("name", "Call of Duty: Warzone");
            gObj->setProperty("supportedVersion", "Legacy / Auto");
            gObj->setProperty("tacticalMode", "Default");
            obj->setProperty("game", juce::var(gObj));
        }

        // Si no tiene sección headphone, inyectar auricular genérico no calibrado
        if (!obj->hasProperty("headphone"))
        {
            auto* hObj = new juce::DynamicObject();
            hObj->setProperty("manufacturer", "Generic");
            hObj->setProperty("model", "Headphones");
            hObj->setProperty("variant", "Default");
            hObj->setProperty("calibrationStatus", "uncalibrated");
            hObj->setProperty("targetCurve", "Harman Over-Ear 2018");
            hObj->setProperty("measurementSource", "None");
            hObj->setProperty("measurementDate", "");
            obj->setProperty("headphone", juce::var(hObj));
        }

        // Si no tiene spatial, inyectar sección desactivada por defecto
        if (!obj->hasProperty("spatial"))
        {
            auto* sObj = new juce::DynamicObject();
            sObj->setProperty("enabled", false);
            sObj->setProperty("soundstageWidth", 1.0);
            sObj->setProperty("apparentDistanceM", 1.4);
            sObj->setProperty("frontBackSeparation", 1.0);
            sObj->setProperty("elevationEmphasis", 0.0);
            sObj->setProperty("bypass", false);
            obj->setProperty("spatial", juce::var(sObj));
        }

        // Normalizar metadatos
        if (!obj->hasProperty("metadata"))
        {
            auto* mObj = new juce::DynamicObject();
            mObj->setProperty("author", "DashAudio");
            mObj->setProperty("license", "CC-BY-4.0");
            mObj->setProperty("version", "1.0.0");
            mObj->setProperty("createdAt", "2026-10-30");
            mObj->setProperty("changelog", "Migrated from unversioned legacy profile");
            mObj->setProperty("verified", false);
            obj->setProperty("metadata", juce::var(mObj));
        }
    }

    outMigratedJson = juce::JSON::toString(root, false).toStdString();
    return true;
}

bool ProfileMigrator::migrateProfile(AudioProfile& profile, std::string* outError)
{
    if (profile.schemaVersion == AudioProfile::kCurrentSchemaVersion)
        return true;

    if (!canMigrate(profile.schemaVersion))
    {
        if (outError)
            *outError = "Versión de perfil no soportada para migración: " + std::to_string(profile.schemaVersion);
        return false;
    }

    profile.schemaVersion = AudioProfile::kCurrentSchemaVersion;
    return true;
}

} // namespace dash::profile
