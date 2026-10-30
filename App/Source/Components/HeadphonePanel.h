#pragma once

#include <JuceHeader.h>
#include <functional>

namespace dash::ui
{

/**
 * Panel de Calibración Acústica por Auricular (HeadphonePanel).
 *
 * Muestra el estado de calibración verificada (AutoEq hacia curva Harman 2018)
 * y el estado explícito no calibrado para modelos sin medición oficial de laboratorio
 * (ej: Razer BlackShark V2 X).
 */
class HeadphonePanel final : public juce::Component
{
public:
    HeadphonePanel();
    ~HeadphonePanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(const juce::String& headphoneModelId)> onHeadphoneModelChanged;

private:
    void updateModelInfo();

    juce::Label titleLabel_;
    juce::Label modelSelectLabel_;
    juce::ComboBox modelSelector_;

    juce::Label statusBadgeLabel_;
    juce::Label manufacturerLabel_;
    juce::Label targetCurveLabel_;
    juce::Label measurementSourceLabel_;
    juce::Label licenseLabel_;
    juce::Label descriptionLabel_;

    juce::ToggleButton enableCompensationToggle_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeadphonePanel)
};

} // namespace dash::ui
