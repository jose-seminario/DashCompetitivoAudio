#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class LoudnessCompressorAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit LoudnessCompressorAudioProcessorEditor(LoudnessCompressorAudioProcessor&);
    ~LoudnessCompressorAudioProcessorEditor() override;

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

    LoudnessCompressorAudioProcessor& processorRef_;

    juce::Label headerLabel_;
    juce::Label meterReadoutLabel_;
    juce::ToggleButton bypassButton_ { "Bypass DSP" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment_;

    RotaryControl amountCtrl_;
    RotaryControl targetCtrl_;
    RotaryControl releaseCtrl_;
    RotaryControl detRelCtrl_;
    RotaryControl limRelCtrl_;
    RotaryControl scLowCtrl_;
    RotaryControl scHighCtrl_;
    RotaryControl liftCtrl_;
    RotaryControl gateCtrl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoudnessCompressorAudioProcessorEditor)
};
