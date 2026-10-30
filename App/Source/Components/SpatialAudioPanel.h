#pragma once

#include <JuceHeader.h>
#include <DashAudioSpatial/SpatialProcessor.h>

namespace dash::ui
{

/**
 * Panel de Control Espacial Tridimensional y HRTF/HRIR (SpatialAudioPanel).
 *
 * Configura la apertura estéreo, distancia acústica aparente, discriminación
 * frente/detrás, elevación y modelo de difracción binaural para auriculares.
 */
class SpatialAudioPanel final : public juce::Component
{
public:
    SpatialAudioPanel();
    ~SpatialAudioPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setParameters(const dash::spatial::SpatialProcessorParameters& params);
    [[nodiscard]] dash::spatial::SpatialProcessorParameters getParameters() const;

    std::function<void(const dash::spatial::SpatialProcessorParameters&)> onParametersChanged;

private:
    void initKnob(juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double def, const juce::String& suffix);
    void drawTopDownSpeakerLayout(juce::Graphics& g, const juce::Rectangle<float>& area);

    dash::spatial::SpatialProcessorParameters params_;

    juce::Label titleLabel_;
    juce::ToggleButton bypassToggle_;

    juce::Label hrtfModelLabel_;
    juce::ComboBox hrtfModelSelector_;

    juce::Slider widthSlider_;
    juce::Label  widthLabel_;

    juce::Slider distanceSlider_;
    juce::Label  distanceLabel_;

    juce::Slider frontBackSlider_;
    juce::Label  frontBackLabel_;

    juce::Slider elevationSlider_;
    juce::Label  elevationLabel_;

    juce::Label lfeBypassNoteLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialAudioPanel)
};

} // namespace dash::ui
