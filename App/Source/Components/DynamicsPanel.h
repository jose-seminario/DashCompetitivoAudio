#pragma once

#include <JuceHeader.h>
#include <DashAudioCore/DynamicsProcessor.h>

namespace dash::ui
{

/**
 * Panel de Dinámica y Compresión Híbrida ITU-R BS.1770-4 (DynamicsPanel).
 *
 * Ofrece control exhaustivo sobre Target LUFS, ratio, envolventes de ataque/relajación,
 * compresión ascendente/descendente y curva de transferencia estática.
 */
class DynamicsPanel final : public juce::Component
{
public:
    DynamicsPanel();
    ~DynamicsPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setParameters(const dash::core::DynamicsParameters& params);
    [[nodiscard]] dash::core::DynamicsParameters getParameters() const;

    std::function<void(const dash::core::DynamicsParameters&)> onParametersChanged;

private:
    void initKnob(juce::Slider& s, juce::Label& l, const juce::String& name, double min, double max, double def, const juce::String& suffix);
    void drawTransferCurve(juce::Graphics& g, const juce::Rectangle<float>& area);

    dash::core::DynamicsParameters params_;

    juce::Label titleLabel_;
    juce::ToggleButton bypassToggle_;

    juce::Slider targetLufsSlider_;
    juce::Label  targetLufsLabel_;

    juce::Slider ratioSlider_;
    juce::Label  ratioLabel_;

    juce::Slider attackSlider_;
    juce::Label  attackLabel_;

    juce::Slider releaseSlider_;
    juce::Label  releaseLabel_;

    juce::Slider upwardSlider_;
    juce::Label  upwardLabel_;

    juce::Slider downwardSlider_;
    juce::Label  downwardLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicsPanel)
};

} // namespace dash::ui
