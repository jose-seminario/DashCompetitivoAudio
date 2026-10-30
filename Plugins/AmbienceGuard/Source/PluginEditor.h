#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AmbienceGuardAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit AmbienceGuardAudioProcessorEditor(AmbienceGuardAudioProcessor&);
    ~AmbienceGuardAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct RotaryControl
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void initRotary(RotaryControl& ctrl, const juce::String& paramId, const juce::String& title);

    AmbienceGuardAudioProcessor& processorRef_;

    juce::Label headerLabel_;
    juce::Label meterLabel_;

    juce::ToggleButton bandModButton_ { "Multiband (BandMod)" };
    juce::ToggleButton distantButton_ { "Distant Cues Guard" };
    juce::ToggleButton bypassButton_  { "Bypass" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandModAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> distantAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment_;

    RotaryControl ambGuarCtrl_;
    RotaryControl targetCtrl_;
    RotaryControl scLowCtrl_;
    RotaryControl scHighCtrl_;
    RotaryControl releaseCtrl_;
    RotaryControl detRelCtrl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmbienceGuardAudioProcessorEditor)
};
