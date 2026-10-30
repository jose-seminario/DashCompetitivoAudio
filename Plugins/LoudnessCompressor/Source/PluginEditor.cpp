#include "PluginEditor.h"

LoudnessCompressorAudioProcessorEditor::LoudnessCompressorAudioProcessorEditor(
    LoudnessCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef_(p)
{
    setSize(720, 420);

    headerLabel_.setText("DASH HYBRID LOUDNESS COMPRESSOR (ITU-R BS.1770)", juce::dontSendNotification);
    headerLabel_.setFont(juce::FontOptions(17.5f, juce::Font::bold));
    headerLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffe6edf3));
    addAndMakeVisible(headerLabel_);

    meterReadoutLabel_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.5f, juce::Font::bold));
    meterReadoutLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff58a6ff));
    meterReadoutLabel_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(meterReadoutLabel_);

    bypassButton_.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffc9d1d9));
    addAndMakeVisible(bypassButton_);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef_.getValueTreeState(),
        LoudnessCompressorAudioProcessor::ParamIDs::bypass,
        bypassButton_);

    using IDs = LoudnessCompressorAudioProcessor::ParamIDs;
    initRotary(amountCtrl_,  IDs::amount,  "Amount");
    initRotary(targetCtrl_,  IDs::target,  "Target (LUFS)");
    initRotary(releaseCtrl_, IDs::release, "Release");
    initRotary(detRelCtrl_,  IDs::detRel,  "Det Release");
    initRotary(limRelCtrl_,  IDs::limRel,  "Lim Release");
    initRotary(scLowCtrl_,   IDs::scLow,   "SC Low");
    initRotary(scHighCtrl_,  IDs::scHigh,  "SC High");
    initRotary(liftCtrl_,    IDs::lift,    "Upward Lift (dB)");
    initRotary(gateCtrl_,    IDs::gate,    "Noise Gate (LUFS)");

    startTimerHz(30);
}

LoudnessCompressorAudioProcessorEditor::~LoudnessCompressorAudioProcessorEditor()
{
    stopTimer();
}

void LoudnessCompressorAudioProcessorEditor::initRotary(
    RotaryControl& ctrl,
    const juce::String& paramId,
    const juce::String& title)
{
    ctrl.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ctrl.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 20);
    ctrl.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff2f81f7));
    ctrl.slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff79c0ff));
    ctrl.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff30363d));
    addAndMakeVisible(ctrl.slider);

    ctrl.label.setText(title, juce::dontSendNotification);
    ctrl.label.setJustificationType(juce::Justification::centred);
    ctrl.label.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    ctrl.label.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(ctrl.label);

    ctrl.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef_.getValueTreeState(),
        paramId,
        ctrl.slider);
}

void LoudnessCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d1117));

    auto panelBounds = getLocalBounds().reduced(14);
    panelBounds.removeFromTop(46);

    g.setColour(juce::Colour(0xff161b22));
    g.fillRoundedRectangle(panelBounds.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff30363d));
    g.drawRoundedRectangle(panelBounds.toFloat(), 8.0f, 1.0f);
}

void LoudnessCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);

    auto topBar = area.removeFromTop(34);
    bypassButton_.setBounds(topBar.removeFromRight(115));
    meterReadoutLabel_.setBounds(topBar.removeFromRight(250));
    headerLabel_.setBounds(topBar);

    area.removeFromTop(18);

    RotaryControl* row1[5] = { &amountCtrl_, &targetCtrl_, &liftCtrl_, &releaseCtrl_, &detRelCtrl_ };
    RotaryControl* row2[4] = { &limRelCtrl_, &scLowCtrl_, &scHighCtrl_, &gateCtrl_ };

    auto topKnobs = area.removeFromTop(area.getHeight() / 2);
    auto botKnobs = area;

    const int w1 = topKnobs.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
    {
        auto cell = topKnobs.removeFromLeft(w1).reduced(6);
        row1[i]->label.setBounds(cell.removeFromTop(20));
        row1[i]->slider.setBounds(cell);
    }

    const int w2 = botKnobs.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = botKnobs.removeFromLeft(w2).reduced(6);
        row2[i]->label.setBounds(cell.removeFromTop(20));
        row2[i]->slider.setBounds(cell);
    }
}

void LoudnessCompressorAudioProcessorEditor::timerCallback()
{
    const float lufs   = processorRef_.getMeterLufs();
    const float gainDb = processorRef_.getMeterGainDb();

    juce::String text;
    text << "BS.1770: " << juce::String(lufs, 1) << " LUFS  |  Gain: "
         << (gainDb >= 0.0f ? "+" : "") << juce::String(gainDb, 1) << " dB";

    meterReadoutLabel_.setText(text, juce::dontSendNotification);
}
