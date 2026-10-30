#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TransientSuppressorAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit TransientSuppressorAudioProcessorEditor(TransientSuppressorAudioProcessor&);
    ~TransientSuppressorAudioProcessorEditor() override;

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

    TransientSuppressorAudioProcessor& processorRef_;

    juce::Label headerLabel_;
    juce::Label meterLabel_;

    juce::ToggleButton frontButton_  { "Front / Center Only" };
    juce::ToggleButton bypassButton_ { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> frontAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment_;

    RotaryControl gunCtrl_;
    RotaryControl releaseCtrl_;
    RotaryControl stepsCtrl_;
    RotaryControl restoreCtrl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientSuppressorAudioProcessorEditor)
};
