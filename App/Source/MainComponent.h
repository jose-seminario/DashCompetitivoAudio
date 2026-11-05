#pragma once

#include <JuceHeader.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "Theme/DashLookAndFeel.h"
#include "Core/AppSettingsStore.h"
#include "Core/AudioPolicyManager.h"
#include "Core/ApoConfigInjector.h"
#include "Core/ApoConfigManager.h"
#include "Audio/JuceWasapiAudioBackend.h"
#include <DashAudioCore/AudioConstants.h>
#include <DashAudioCore/AudioGraph.h>
#include <DashAudioCore/ParametricEQProcessor.h>

namespace dash::ui
{

/**
 * Componente Principal del Ecualizador Táctico Cyberpunk / Sci-Fi HUD (MainComponent).
 *
 * Arquitectura HUD Gamer:
 *  - Barra Superior de Telemetría (Top Bar): Estado APO, CPU %, Latencia ms, Bypass.
 *  - Sidebar Táctico Izquierdo: Navegación biselada, Control Preamp Maestro y Reset.
 *  - Pantalla Central HUD Sci-Fi: Matriz de 6 bandas paramétricas con faders de ±18 dB
 *    y marco cibernético angular con esquinas a 45°, scanlines y brackets.
 */
class MainComponent final
    : public juce::Component,
      private juce::Timer
{
public:
    enum class ActiveView
    {
        Equalizer,
        Settings
    };

    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class EqualizerHudView;
    class SettingsHudView;
    class ApoOnboardingOverlay;

    struct OutputEndpointEntry
    {
        std::wstring id;
        std::wstring friendlyName;
        std::wstring endpointGuid;
    };

    void timerCallback() override;

    void setView(ActiveView view);
    void updateTelemetryVisuals(float cpuPercent, float latencyMs, bool apoActive);
    void updateNavigationButtons();

    void populateOutputDevices();
    void restartAudioStream();
    void syncDspParameters();
    void resetAllEqAndPreamp();
    void loadPersistedSettings();
    void savePersistedSettings();
    void scheduleDebouncedSettingsSave();
    void scheduleDebouncedApoSync();
    bool executeApoSync(std::string* outError = nullptr);
    void checkApoInstallation(bool userInitiated = false);

    DashLookAndFeel dashLookAndFeel_;

    dash::audio::AudioPolicyManager                      policyManager_;
    dash::core::AudioGraphParameters                     graphParams_;
    std::unique_ptr<dash::core::AudioGraph>              audioGraph_;
    std::unique_ptr<dash::audio::JuceWasapiAudioBackend> audioBackend_;

    std::vector<OutputEndpointEntry> outputEndpoints_;
    std::wstring selectedDeviceName_;
    std::string  selectedEndpointGuid_;

    bool needsApoSync_                 = false;
    int  debounceCounter_              = 0;
    bool needsSettingsSave_            = false;
    int  settingsSaveDebounceCounter_  = 0;

    ActiveView currentView_ { ActiveView::Equalizer };

    // --- Controles de la Barra Superior (Top Bar Telemetría) ---
    juce::Rectangle<int> topBarBounds_;
    juce::Label          appTitleLabel_;
    juce::Label          cpuChipLabel_;
    juce::Label          latencyChipLabel_;
    juce::Label          apoChipLabel_;
    juce::ToggleButton   masterBypassToggle_;

    // --- Controles del Sidebar Táctico Izquierdo ---
    juce::Rectangle<int> sidebarBounds_;
    juce::Label          navHeaderLabel_;
    juce::TextButton     navEqButton_;
    juce::TextButton     navSettingsButton_;

    juce::Rectangle<int> preampCardBounds_;
    juce::Label          preampTitleLabel_;
    juce::Label          preampSubLabel_;
    juce::Slider         preampSlider_;
    juce::TextButton     resetButton_;

    juce::Rectangle<int> deviceCardBounds_;
    juce::Label          deviceHeaderLabel_;
    juce::Label          deviceActiveLabel_;

    // --- Vistas Centrales HUD y Overlay ---
    juce::Rectangle<int> centralHudBounds_;
    std::unique_ptr<EqualizerHudView>     equalizerHudView_;
    std::unique_ptr<SettingsHudView>      settingsHudView_;
    std::unique_ptr<ApoOnboardingOverlay> apoOverlay_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace dash::ui
