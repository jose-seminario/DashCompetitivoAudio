#include "AppSettingsStore.h"

#include <algorithm>
#include <cmath>

namespace dash::audio
{

namespace
{
    float clampEqDb(double rawValue) noexcept
    {
        if (!std::isfinite(rawValue))
            return 0.0f;

        return std::clamp(
            static_cast<float>(rawValue),
            dash::core::kMinEqGainDb,
            dash::core::kMaxEqGainDb);
    }
} // namespace

juce::File AppSettingsStore::getDefaultSettingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("DashAudioSettings.json");
}

bool AppSettingsStore::save(
    const AppPersistedSettings& settings,
    const juce::File& targetFile)
{
    const juce::File parentDir = targetFile.getParentDirectory();
    if (!parentDir.isDirectory() && !parentDir.createDirectory())
        return false;

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("preampDb", static_cast<double>(clampEqDb(static_cast<double>(settings.preampDb))));

    juce::Array<juce::var> bandsArray;
    bandsArray.ensureStorageAllocated(static_cast<int>(settings.bandGainsDb.size()));
    for (float gainDb : settings.bandGainsDb)
        bandsArray.add(static_cast<double>(clampEqDb(static_cast<double>(gainDb))));

    root->setProperty("bandGainsDb", juce::var(bandsArray));
    root->setProperty("bypass", settings.bypass);

    const juce::String jsonText = juce::JSON::toString(juce::var(root.get()));

    juce::TemporaryFile tempFile(targetFile);
    if (tempFile.getFile().replaceWithText(jsonText))
    {
        if (tempFile.overwriteTargetFileWithTemporary())
            return true;
    }

    return targetFile.replaceWithText(jsonText);
}

bool AppSettingsStore::load(
    AppPersistedSettings& outSettings,
    const juce::File& sourceFile)
{
    outSettings = AppPersistedSettings {};

    if (!sourceFile.existsAsFile())
        return false;

    const juce::var parsed = juce::JSON::parse(sourceFile);
    const auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return false;

    if (obj->hasProperty("preampDb"))
    {
        const juce::var& v = obj->getProperty("preampDb");
        if (v.isDouble() || v.isInt() || v.isInt64())
            outSettings.preampDb = clampEqDb(static_cast<double>(v));
    }

    if (obj->hasProperty("bandGainsDb"))
    {
        if (const auto* arr = obj->getProperty("bandGainsDb").getArray())
        {
            const size_t count = std::min(
                static_cast<size_t>(arr->size()),
                outSettings.bandGainsDb.size());

            for (size_t i = 0; i < count; ++i)
            {
                const juce::var& item = (*arr)[static_cast<int>(i)];
                if (item.isDouble() || item.isInt() || item.isInt64())
                    outSettings.bandGainsDb[i] = clampEqDb(static_cast<double>(item));
            }
        }
    }

    if (obj->hasProperty("bypass"))
    {
        const juce::var& v = obj->getProperty("bypass");
        if (v.isBool() || v.isInt() || v.isInt64())
            outSettings.bypass = static_cast<bool>(v);
    }

    return true;
}

} // namespace dash::audio
