#include "PluginEditor.h"

AmbienceGuardAudioProcessorEditor::AmbienceGuardAudioProcessorEditor(
    AmbienceGuardAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef_(p)
{
    setSize(720, 400);

    headerLabel_.setText("DASH AMBIENCE FREQUENCY GUARD (MULTIBAND IIR)", juce::dontSendNotification);
    headerLabel_.setFont(juce::FontOptions(16.5f, juce::Font::bold));
    headerLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f4f5));
    addAndMakeVisible(headerLabel_);

    meterLabel_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.5f, juce::Font::bold));
    meterLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    meterLabel_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(meterLabel_);

    for (auto* btn : { &bandModButton_, &distantButton_, &bypassButton_ })
    {
        btn->setColour(juce::ToggleButton::textColourId, juce::Colour(0xffd4d4d8));
        addAndMakeVisible(btn);
    }

    using IDs = AmbienceGuardAudioProcessor::ParamIDs;
    bandModAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef_.getValueTreeState(), IDs::bandMod, bandModButton_);
    distantAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef_.getValueTreeState(), IDs::distant, distantButton_);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef_.getValueTreeState(), IDs::bypass, bypassButton_);

    initRotary(ambGuarCtrl_, IDs::ambGuar, "AmbGuar (1.0)");
    initRotary(targetCtrl_,  IDs::target,  "Target (0.388889)");
    initRotary(scLowCtrl_,   IDs::scLow,   "SCLow (350 Hz)");
    initRotary(scHighCtrl_,  IDs::scHigh,  "SCHigh (2.5 kHz)");
    initRotary(releaseCtrl_, IDs::release, "Release");
    initRotary(detRelCtrl_,  IDs::detRel,  "DetRel");

    startTimerHz(30);
}

AmbienceGuardAudioProcessorEditor::~AmbienceGuardAudioProcessorEditor()
{
    stopTimer();
}

void AmbienceGuardAudioProcessorEditor::initRotary(
    RotaryControl& ctrl,
    const juce::String& paramId,
    const juce::String& title)
{
    ctrl.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    ctrl.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
    ctrl.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff0284c7));
    ctrl.slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff38bdf8));
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

void AmbienceGuardAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto card = getLocalBounds().reduced(14);
    card.removeFromTop(44);

    g.setColour(juce::Colour(0xff252528));
    g.fillRoundedRectangle(card.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff38383c));
    g.drawRoundedRectangle(card.toFloat(), 8.0f, 1.2f);
}

void AmbienceGuardAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);

    auto topRow = area.removeFromTop(32);
    meterLabel_.setBounds(topRow.removeFromRight(290));
    headerLabel_.setBounds(topRow);

    area.removeFromTop(18);

    auto toggleRow = area.removeFromBottom(30);
    bandModButton_.setBounds(toggleRow.removeFromLeft(190));
    distantButton_.setBounds(toggleRow.removeFromLeft(190));
    bypassButton_.setBounds(toggleRow.removeFromRight(100));

    area.removeFromBottom(8);

    RotaryControl* row1[3] = { &ambGuarCtrl_, &targetCtrl_, &releaseCtrl_ };
    RotaryControl* row2[3] = { &scLowCtrl_, &scHighCtrl_, &detRelCtrl_ };

    auto topKnobs = area.removeFromTop(area.getHeight() / 2);
    auto botKnobs = area;

    const int colW = topKnobs.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto c1 = topKnobs.removeFromLeft(colW).reduced(8, 2);
        row1[i]->label.setBounds(c1.removeFromTop(18));
        row1[i]->slider.setBounds(c1);

        auto c2 = botKnobs.removeFromLeft(colW).reduced(8, 2);
        row2[i]->label.setBounds(c2.removeFromTop(18));
        row2[i]->slider.setBounds(c2);
    }
}

void AmbienceGuardAudioProcessorEditor::timerCallback()
{
    const float hpHz   = processorRef_.getMeterHpHz();
    const float lpKhz  = processorRef_.getMeterLpHz() * 0.001f;
    const float gainDb = processorRef_.getMeterGainDb();

    juce::String msg;
    msg << "SC: " << juce::String(hpHz, 0) << "Hz - " << juce::String(lpKhz, 2)
        << "kHz  |  GR: " << juce::String(gainDb, 1) << " dB";
    meterLabel_.setText(msg, juce::dontSendNotification);
}
