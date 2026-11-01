#include "TacticalProcessingPanel.h"
#include "../Theme/DashLookAndFeel.h"

#include <cmath>

namespace dash::ui
{

TacticalProcessingPanel::TacticalProcessingPanel()
{
    // 1. Transient Suppressor
    transientHeader_.setText(juce::String::fromUTF8(u8"1. SUPRESOR DE DISPAROS (GUN CRACK REDUCTION)"), juce::dontSendNotification);
    transientHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f).withStyle("Bold")));
    transientHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(transientHeader_);

    transientBypass_.setButtonText("Bypass");
    transientBypass_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    transientBypass_.onClick = [this] { notifyChange(); };
    addAndMakeVisible(transientBypass_);

    initKnob(gunReductionSlider_, gunReductionLabel_, juce::String::fromUTF8(u8"Reducción Disparos (Gun)"), 0.0, 1.0, 0.45, "");
    initKnob(gunCrossoverSlider_, gunCrossoverLabel_, "Crossover Linkwitz-Riley", 150.0, 1200.0, 280.0, " Hz");
    initKnob(gunAttackSlider_, gunAttackLabel_, juce::String::fromUTF8(u8"Ataque Envolvente"), 0.05, 10.0, 0.20, " ms");
    initKnob(gunReleaseSlider_, gunReleaseLabel_, juce::String::fromUTF8(u8"Relajación"), 5.0, 250.0, 45.0, " ms");

    // 2. Ambience Guard
    ambHeader_.setText(juce::String::fromUTF8(u8"2. PROTECTOR AMBIENTAL MULTIBANDA (AMBIENCE GUARD)"), juce::dontSendNotification);
    ambHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f).withStyle("Bold")));
    ambHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(ambHeader_);

    ambBypass_.setButtonText("Bypass");
    ambBypass_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    ambBypass_.onClick = [this] { notifyChange(); };
    addAndMakeVisible(ambBypass_);

    initKnob(ambReductionSlider_, ambReductionLabel_, juce::String::fromUTF8(u8"Protección (AmbGuar)"), 0.0, 1.0, 1.0, "");
    initKnob(ambLowCutSlider_, ambLowCutLabel_, "Corte Inferior", 100.0, 600.0, 350.0, " Hz");
    initKnob(ambHighCutSlider_, ambHighCutLabel_, "Corte Superior", 1200.0, 4500.0, 2500.0, " Hz");

    // 3. Limiter Processor
    limHeader_.setText(juce::String::fromUTF8(u8"3. LIMITADOR DE PICO ABSOLUTO (PEAK LIMITER)"), juce::dontSendNotification);
    limHeader_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f).withStyle("Bold")));
    limHeader_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(limHeader_);

    limBypass_.setButtonText("Bypass");
    limBypass_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    limBypass_.onClick = [this] { notifyChange(); };
    addAndMakeVisible(limBypass_);

    initKnob(limCeilingSlider_, limCeilingLabel_, juce::String::fromUTF8(u8"Techo Límite (Ceiling)"), -6.0, 0.0, -0.5, " dBFS");
    initKnob(limReleaseSlider_, limReleaseLabel_, juce::String::fromUTF8(u8"Tiempo Relajación"), 5.0, 200.0, 50.0, " ms");
}

void TacticalProcessingPanel::initKnob(
    juce::Slider& s, juce::Label& l, const juce::String& text,
    double min, double max, double def, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    s.setRange(min, max, (max <= 1.0) ? 0.01 : 0.1);
    s.setValue(def, juce::dontSendNotification);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    s.onValueChange = [this] { notifyChange(); };
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(l);
}

void TacticalProcessingPanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    const float cardW = (float)getWidth() - 32.0f;

    // Tarjeta 1: Transient
    const auto card1 = juce::Rectangle<float>(16, 12, cardW, 140);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(card1, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(card1, 8.0f, 1.2f);

    // Tarjeta 2: Ambience
    const auto card2 = juce::Rectangle<float>(16, 164, cardW, 140);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(card2, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(card2, 8.0f, 1.2f);

    // Tarjeta 3: Limiter
    const auto card3 = juce::Rectangle<float>(16, 316, cardW, 130);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(card3, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(card3, 8.0f, 1.2f);
}

void TacticalProcessingPanel::resized()
{
    const int startX = 36;
    const int knobW = 120;
    const int knobH = 92;

    // Fila 1: Transient
    transientHeader_.setBounds(startX, 20, 420, 20);
    transientBypass_.setBounds(getWidth() - 140, 20, 100, 20);

    const int row1DialY = 46;
    gunReductionLabel_.setBounds(startX, row1DialY, knobW, 16);
    gunReductionSlider_.setBounds(startX, row1DialY + 16, knobW, knobH);

    gunCrossoverLabel_.setBounds(startX + 140, row1DialY, knobW, 16);
    gunCrossoverSlider_.setBounds(startX + 140, row1DialY + 16, knobW, knobH);

    gunAttackLabel_.setBounds(startX + 280, row1DialY, knobW, 16);
    gunAttackSlider_.setBounds(startX + 280, row1DialY + 16, knobW, knobH);

    gunReleaseLabel_.setBounds(startX + 420, row1DialY, knobW, 16);
    gunReleaseSlider_.setBounds(startX + 420, row1DialY + 16, knobW, knobH);

    // Fila 2: Ambience
    ambHeader_.setBounds(startX, 172, 420, 20);
    ambBypass_.setBounds(getWidth() - 140, 172, 100, 20);

    const int row2DialY = 198;
    ambReductionLabel_.setBounds(startX, row2DialY, knobW, 16);
    ambReductionSlider_.setBounds(startX, row2DialY + 16, knobW, knobH);

    ambLowCutLabel_.setBounds(startX + 140, row2DialY, knobW, 16);
    ambLowCutSlider_.setBounds(startX + 140, row2DialY + 16, knobW, knobH);

    ambHighCutLabel_.setBounds(startX + 280, row2DialY, knobW, 16);
    ambHighCutSlider_.setBounds(startX + 280, row2DialY + 16, knobW, knobH);

    // Fila 3: Limiter
    limHeader_.setBounds(startX, 324, 420, 20);
    limBypass_.setBounds(getWidth() - 140, 324, 100, 20);

    const int row3DialY = 348;
    limCeilingLabel_.setBounds(startX, row3DialY, knobW, 16);
    limCeilingSlider_.setBounds(startX, row3DialY + 16, knobW, knobH);

    limReleaseLabel_.setBounds(startX + 140, row3DialY, knobW, 16);
    limReleaseSlider_.setBounds(startX + 140, row3DialY + 16, knobW, knobH);
}

void TacticalProcessingPanel::notifyChange()
{
    if (onParametersChanged)
    {
        dash::core::TransientParameters t;
        t.gun              = static_cast<float>(gunReductionSlider_.getValue());
        t.frequencySplitHz = static_cast<float>(gunCrossoverSlider_.getValue());
        t.attackMs         = static_cast<float>(gunAttackSlider_.getValue());
        t.releaseMs        = static_cast<float>(gunReleaseSlider_.getValue());
        t.bypass           = transientBypass_.getToggleState();

        dash::core::AmbienceParameters a;
        a.ambGuar = static_cast<float>(ambReductionSlider_.getValue());
        a.scLow   = juce::jlimit(0.0f, 1.0f, std::log10(static_cast<float>(ambLowCutSlider_.getValue()) / 20.0f) / 3.0f);
        a.scHigh  = juce::jlimit(0.0f, 1.0f, std::log10(static_cast<float>(ambHighCutSlider_.getValue()) / 20.0f) / 3.0f);
        a.bypass  = ambBypass_.getToggleState();

        dash::core::LimiterParameters l;
        l.ceilingDb = static_cast<float>(limCeilingSlider_.getValue());
        l.releaseMs = static_cast<float>(limReleaseSlider_.getValue());
        l.bypass    = limBypass_.getToggleState();

        onParametersChanged(t, a, l);
    }
}

void TacticalProcessingPanel::setParameters(
    const dash::core::TransientParameters& trans,
    const dash::core::AmbienceParameters& amb,
    const dash::core::LimiterParameters& lim)
{
    gunReductionSlider_.setValue(trans.gun, juce::dontSendNotification);
    gunCrossoverSlider_.setValue(trans.frequencySplitHz, juce::dontSendNotification);
    gunAttackSlider_.setValue(trans.attackMs, juce::dontSendNotification);
    gunReleaseSlider_.setValue(trans.releaseMs, juce::dontSendNotification);
    transientBypass_.setToggleState(trans.bypass, juce::dontSendNotification);

    ambReductionSlider_.setValue(amb.ambGuar, juce::dontSendNotification);
    ambLowCutSlider_.setValue(dash::core::AmbienceProcessor::mapScNormToHz(amb.scLow), juce::dontSendNotification);
    ambHighCutSlider_.setValue(dash::core::AmbienceProcessor::mapScNormToHz(amb.scHigh), juce::dontSendNotification);
    ambBypass_.setToggleState(amb.bypass, juce::dontSendNotification);

    limCeilingSlider_.setValue(lim.ceilingDb, juce::dontSendNotification);
    limReleaseSlider_.setValue(lim.releaseMs, juce::dontSendNotification);
    limBypass_.setToggleState(lim.bypass, juce::dontSendNotification);
}

} // namespace dash::ui
