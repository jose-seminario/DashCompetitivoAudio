#pragma once

#include <JuceHeader.h>
#include <filesystem>
#include <string>

#include "Core/AudioPolicyManager.h"
#include "Core/ApoConfigInjector.h"
#include "Core/ApoConfigManager.h"
#include "Core/SystemDependencyChecker.h"

namespace dash::ui
{

class MainComponent final
    : public juce::Component,
      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    static constexpr int kDebounceMs = 120;

    void timerCallback() override;

    void initRotarySlider(
        juce::Slider& slider,
        juce::Label& label,
        const juce::String& titleUtf8,
        double minVal,
        double maxVal,
        double step,
        double defaultVal,
        int decimalPlaces,
        const juce::String& suffix = {},
        juce::Colour accentColour = juce::Colour(0xff0284c7));

    void applyPresetValuesToSliders(int profileId, bool triggerInjection);
    void scheduleDebouncedInjection();
    void rebuildAndInjectConfig(bool forcePermissionPopup = false);

    void activateTacticalProfile();
    void deactivateAndRestoreAudio();

    void refreshSystemDependencies();
    void onInstallDependenciesClicked();
    void handlePermissionDeniedAlert(bool forcePopup);
    void setStatusMessage(const juce::String& textUtf8, bool isError = false);

    // Motores nativos de sistema (COM + Equalizer APO + Checker)
    dash::audio::AudioPolicyManager policyManager_;
    dash::audio::ApoChainConfig apoConfig_;
    dash::audio::DependencyStatus depStatus_;

    std::filesystem::path apoConfigPath_;
    std::wstring originalEndpointId_;
    std::wstring originalEndpointName_;
    bool isProfileActive_          = false;
    bool isUpdatingPresetSliders_  = false;
    bool hasShownAdminPopup_       = false;

    // 1. Encabezado Principal
    juce::Label titleLabel_;
    juce::Label subtitleLabel_;

    // 2. Panel de Perfiles y Orquestación WASAPI
    juce::Label profileSectionHeader_;
    juce::Label profileSelectLabel_;
    juce::ComboBox profileSelector_;
    juce::TextButton activateProfileBtn_;
    juce::TextButton deactivateProfileBtn_;
    juce::Label routeInfoLabel_;

    // 3. Panel de Compresor (Fila 1) y Personalización Extrema Warzone (Fila 2)
    juce::Label compressorSectionHeader_;
    juce::Label warzoneSectionHeader_;

    juce::Slider amountSlider_;
    juce::Label  amountLabel_;

    juce::Slider targetSlider_;
    juce::Label  targetLabel_;
    juce::Label  targetLufsBadge_;

    juce::Slider releaseSlider_;
    juce::Label  releaseLabel_;

    juce::Slider gunSlider_;
    juce::Label  gunLabel_;

    juce::Slider footstepsSlider_;
    juce::Label  footstepsLabel_;

    juce::Slider bassSlider_;
    juce::Label  bassLabel_;

    juce::Slider ambienceSlider_;
    juce::Label  ambienceLabel_;

    // 4. Panel "Configuración del Sistema" (System Checker + Asistente)
    juce::Label sysConfigSectionHeader_;
    juce::Label depStatusLabel_;
    juce::TextButton installDepsBtn_;
    juce::TextButton recheckDepsBtn_;
    juce::TextButton runAsAdminBtn_;
    juce::TextEditor installGuideEditor_;

    // 5. Barra de Estado Inferior
    juce::Label statusLabel_;

    // Áreas cacheadas para dibujo de tarjetas oscuras
    juce::Rectangle<int> profilePanelBounds_;
    juce::Rectangle<int> dspPanelBounds_;
    juce::Rectangle<int> sysConfigPanelBounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace dash::ui
