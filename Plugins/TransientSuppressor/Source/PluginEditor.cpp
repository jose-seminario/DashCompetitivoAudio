#include "PluginEditor.h"

TransientSuppressorAudioProcessorEditor::TransientSuppressorAudioProcessorEditor(
    TransientSuppressorAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef_(p)
{
    setSize(640, 320);

    headerLabel_.setText("DASH TRANSIENT SUPPRESSOR (CENTER GUN CRACK)", juce::dontSendNotification);
    headerLabel_.setFont(juce::FontOptions(16.5f, juce::Font::bold));
    headerLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f4f5));
    addAndMakeVisible(headerLabel_);

    meterLabel_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::bold));
    meterLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    meterLabel_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(meterLabel_);

    frontButton_.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffd4d4d8));
    addAndMakeVisible(frontButton_);
    frontAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef_.getValueTreeState(),
        TransientSuppressorAudioProcessor::ParamIDs::front,
        frontButton_);

    bypassButton_.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffd4d4d8));
    addAndMakeVisible(bypassButton_);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef_.getValueTreeState(),
        TransientSuppressorAudioProcessor::ParamIDs::bypass,
        bypassButton_);

    using IDs = TransientSuppressorAudioProcessor::ParamIDs;
    initRotary(gunCtrl_,     IDs::gun,     "Gun Ceiling (0.45 = -7dB)");
    initRotary(releaseCtrl_, IDs::release, "Release");
    initRotary(stepsCtrl_,   IDs::steps,   "Steps Guard");
    initRotary(restoreCtrl_, IDs::restore, "Body Restore");

    startTimerHz(30);
}

TransientSuppressorAudioProcessorEditor::~TransientSuppressorAudioProcessorEditor()
{
    stopTimer();
}

void TransientSuppressorAudioProcessorEditor::initRotary(
    RotaryControl& ctrl,
    const juce::String& paramId,
    const juce::String& title)
{
    ctrl.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    ctrl.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 20);
    ctrl.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff10b981));
    ctrl.slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff34d399));
    ctrl.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3f3f46));
    addAndMakeVisible(ctrl.slider);

    ctrl.label.setText(title, juce::dontSendNotification);
    ctrl.label.setJustificationType(juce::Justification::centred);
    ctrl.label.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    ctrl.label.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));
    addAndMakeVisible(ctrl.label);

    ctrl.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef_.getValueTreeState(),
        paramId,
        ctrl.slider);
}

void TransientSuppressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto card = getLocalBounds().reduced(14);
    card.removeFromTop(44);

    g.setColour(juce::Colour(0xff252528));
    g.fillRoundedRectangle(card.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff38383c));
    g.drawRoundedRectangle(card.toFloat(), 8.0f, 1.2f);
}

void TransientSuppressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);

    auto topRow = area.removeFromTop(32);
    meterLabel_.setBounds(topRow.removeFromRight(240));
    headerLabel_.setBounds(topRow);

    area.removeFromTop(18);

    auto bottomToggles = area.removeFromBottom(32);
    frontButton_.setBounds(bottomToggles.removeFromLeft(200));
    bypassButton_.setBounds(bottomToggles.removeFromRight(100));

    area.removeFromBottom(8);

    RotaryControl* knobs[4] = { &gunCtrl_, &releaseCtrl_, &stepsCtrl_, &restoreCtrl_ };
    const int w = area.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = area.removeFromLeft(w).reduced(8, 4);
        knobs[i]->label.setBounds(cell.removeFromTop(20));
        knobs[i]->slider.setBounds(cell);
    }
}

void TransientSuppressorAudioProcessorEditor::timerCallback()
{
    const float grDb = processorRef_.getMeterReductionDb();
    const int ch     = processorRef_.getActiveChannelCount();

    juce::String busText = (ch >= 8) ? "7.1 Center" : (ch >= 6 ? "5.1 Center" : "2.0 Mid");
    juce::String msg;
    msg << "Bus: " << busText << "  |  Crack GR: " << juce::String(grDb, 1) << " dB";
    meterLabel_.setText(msg, juce::dontSendNotification);
}
