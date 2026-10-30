#pragma once

#include <JuceHeader.h>

#include "Core/AudioPolicyManager.h"
#include "Core/ApoConfigInjector.h"

namespace dash::ui
{

class MainComponent final : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void initRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& text,
                          double minVal, double maxVal, double step, double defaultVal,
                          int decimalPlaces);

    // Backend Core (reservado para la orquestación en la Fase 4)
    dash::audio::AudioPolicyManager policyManager_;
    dash::audio::ApoChainConfig apoConfig_;

    // 1. Encabezado Visual
    juce::Label titleLabel_;
    juce::Label subtitleLabel_;

    // 2. Sección de Perfiles de Audio (Fidelidad Rara Audio)
    juce::Label profileSectionHeader_;
    juce::Label profileSelectLabel_;
    juce::ComboBox profileSelector_;
    juce::TextButton activateProfileBtn_;
    juce::TextButton deactivateProfileBtn_;

    // 3. Perillas de Control del Compresor (RotaryVerticalDrag)
    juce::Label compressorSectionHeader_;

    juce::Slider amountSlider_;
    juce::Label amountLabel_;

    juce::Slider targetSlider_;
    juce::Label targetLabel_;
    juce::Label targetLufsBadge_;

    juce::Slider releaseSlider_;
    juce::Label releaseLabel_;

    // 4. Barra de Estado / Telemetría Visual
    juce::Label statusLabel_;

    // Rectángulos cacheados para el renderizado de paneles en paint()
    juce::Rectangle<int> profilePanelBounds_;
    juce::Rectangle<int> compressorPanelBounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace dash::ui
