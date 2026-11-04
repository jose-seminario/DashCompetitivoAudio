#pragma once

#include <juce_core/juce_core.h>

#include <DashAudioCore/AudioConstants.h>
#include <DashAudioCore/ParametricEQProcessor.h>

#include <array>

namespace dash::audio
{

/**
 * Estado persistente del Ecualizador Táctico Estéreo de 6 Bandas.
 */
struct AppPersistedSettings
{
    float preampDb = 0.0f;
    std::array<float, dash::core::ParametricEQProcessor::kMaxBands> bandGainsDb {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    bool bypass = false;
};

/**
 * Gestor de persistencia JSON en el directorio de datos de aplicación del usuario:
 *   %APPDATA%\DashAudioSettings.json (en Windows)
 */
class AppSettingsStore
{
public:
    static juce::File getDefaultSettingsFile();

    static bool save(
        const AppPersistedSettings& settings,
        const juce::File& targetFile = getDefaultSettingsFile());

    static bool load(
        AppPersistedSettings& outSettings,
        const juce::File& sourceFile = getDefaultSettingsFile());
};

} // namespace dash::audio
