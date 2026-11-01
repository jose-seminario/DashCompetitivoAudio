#include "DynamicsPanel.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

DynamicsPanel::DynamicsPanel()
{
    titleLabel_.setText(juce::String::fromUTF8(u8"COMPRESOR HÍBRIDO ITU-R BS.1770-4 (ASCENDENTE / DESCENDENTE)"), juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.0f).withStyle("Bold")));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(titleLabel_);

    bypassToggle_.setButtonText("Bypass Compresor");
    bypassToggle_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    bypassToggle_.onClick = [this] {
        params_.bypass = bypassToggle_.getToggleState();
        if (onParametersChanged) onParametersChanged(params_);
        repaint();
    };
    addAndMakeVisible(bypassToggle_);

    initKnob(targetLufsSlider_, targetLufsLabel_, juce::String::fromUTF8(u8"Target Sonoridad"), -30.0, -8.0, -16.0, " LUFS");
    initKnob(ratioSlider_, ratioLabel_, juce::String::fromUTF8(u8"Ratio Compresión"), 1.0, 10.0, 2.5, " : 1");
    initKnob(attackSlider_, attackLabel_, juce::String::fromUTF8(u8"Tiempo de Ataque"), 0.5, 100.0, 20.0, " ms");
    initKnob(releaseSlider_, releaseLabel_, juce::String::fromUTF8(u8"Tiempo Relajación"), 20.0, 500.0, 120.0, " ms");
    initKnob(upwardSlider_, upwardLabel_, juce::String::fromUTF8(u8"Compresión Ascendente"), 0.0, 1.0, 0.5, "");
    initKnob(downwardSlider_, downwardLabel_, juce::String::fromUTF8(u8"Compresión Descendente"), 0.0, 1.0, 0.5, "");
    initKnob(makeupGainSlider_, makeupGainLabel_, juce::String::fromUTF8(u8"Volumen General (Makeup)"), 0.0, 12.0, 6.0, " dB");
}

void DynamicsPanel::initKnob(
    juce::Slider& s, juce::Label& l, const juce::String& name,
    double min, double max, double def, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    s.setRange(min, max, 0.1);
    s.setValue(def, juce::dontSendNotification);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    s.onValueChange = [this] {
        params_ = getParameters();
        if (onParametersChanged) onParametersChanged(params_);
        repaint();
    };
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(l);
}

void DynamicsPanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    // Tarjeta izquierda (Controles)
    const auto leftCard = juce::Rectangle<float>(16, 44, 460, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(leftCard, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(leftCard, 8.0f, 1.2f);

    // Tarjeta derecha (Curva de transferencia)
    const auto rightCard = juce::Rectangle<float>(490, 44, (float)getWidth() - 506, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(rightCard, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(rightCard, 8.0f, 1.2f);

    // Título de la curva
    g.setColour(DashLookAndFeel::kTextMain);
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f).withStyle("Bold")));
    g.drawText(juce::String::fromUTF8(u8"CURVA ESTÁTICA DE TRANSFERENCIA (I/O dBFS)"), 506, 56, (int)rightCard.getWidth() - 32, 20, juce::Justification::centredLeft);

    drawTransferCurve(g, rightCard.reduced(24.0f).withTrimmedTop(36.0f));
}

void DynamicsPanel::drawTransferCurve(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    // Rejilla de dB (-60 dBFS a 0 dBFS)
    g.setColour(DashLookAndFeel::kBgCardElevated);
    g.fillRect(area);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRect(area, 1.0f);

    for (int db = -50; db <= -10; db += 10)
    {
        const float norm = (db + 60.0f) / 60.0f;
        const float x = area.getX() + norm * area.getWidth();
        const float y = area.getBottom() - norm * area.getHeight();
        g.setColour(DashLookAndFeel::kBorderSubtle.withAlpha(0.4f));
        g.drawVerticalLine((int)x, area.getY(), area.getBottom());
        g.drawHorizontalLine((int)y, area.getX(), area.getRight());
    }

    // Línea 1:1 de referencia diagonal
    g.setColour(DashLookAndFeel::kTextMuted.withAlpha(0.4f));
    g.drawLine(area.getX(), area.getBottom(), area.getRight(), area.getY(), 1.0f);

    // Curva de compresión real calculada
    const float targetLufs = (float)targetLufsSlider_.getValue();
    const float ratio = (float)ratioSlider_.getValue();
    const float makeup = (float)makeupGainSlider_.getValue();

    juce::Path curve;
    bool started = false;

    for (int inDb = -60; inDb <= 0; inDb += 1)
    {
        float outDb = (float)inDb;
        if (inDb > targetLufs)
        {
            // Compresión descendente por encima del target
            outDb = targetLufs + (inDb - targetLufs) / ratio;
        }
        else
        {
            // Compresión ascendente por debajo del target
            const float diff = targetLufs - inDb;
            outDb = inDb + diff * (float)upwardSlider_.getValue() * 0.35f;
        }

        // Aplicar ganancia de maquillaje a la visualización de salida
        outDb += makeup;

        const float normX = (inDb + 60.0f) / 60.0f;
        const float normY = (outDb + 60.0f) / 60.0f;
        const float px = area.getX() + normX * area.getWidth();
        const float py = area.getBottom() - juce::jlimit(0.0f, 1.0f, normY) * area.getHeight();

        if (!started)
        {
            curve.startNewSubPath(px, py);
            started = true;
        }
        else
        {
            curve.lineTo(px, py);
        }
    }

    g.setColour(bypassToggle_.getToggleState() ? DashLookAndFeel::kTextMuted : DashLookAndFeel::kAccentCyan);
    g.strokePath(curve, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void DynamicsPanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);
    bypassToggle_.setBounds(getWidth() - 200, 12, 180, 24);

    const int knobW = 120;
    const int knobH = 100;
    const int startX = 36;

    // Fila 1
    const int row1Y = 70;
    targetLufsLabel_.setBounds(startX, row1Y, knobW, 16);
    targetLufsSlider_.setBounds(startX, row1Y + 18, knobW, knobH);

    ratioLabel_.setBounds(startX + 140, row1Y, knobW, 16);
    ratioSlider_.setBounds(startX + 140, row1Y + 18, knobW, knobH);

    attackLabel_.setBounds(startX + 280, row1Y, knobW, 16);
    attackSlider_.setBounds(startX + 280, row1Y + 18, knobW, knobH);

    // Fila 2
    const int row2Y = 210;
    releaseLabel_.setBounds(startX, row2Y, knobW, 16);
    releaseSlider_.setBounds(startX, row2Y + 18, knobW, knobH);

    upwardLabel_.setBounds(startX + 140, row2Y, knobW, 16);
    upwardSlider_.setBounds(startX + 140, row2Y + 18, knobW, knobH);

    downwardLabel_.setBounds(startX + 280, row2Y, knobW, 16);
    downwardSlider_.setBounds(startX + 280, row2Y + 18, knobW, knobH);

    // Fila 3: Volumen General (Makeup Gain)
    const int row3Y = 350;
    makeupGainLabel_.setBounds(startX, row3Y, knobW + 20, 16);
    makeupGainSlider_.setBounds(startX, row3Y + 18, knobW, knobH);
}

void DynamicsPanel::setParameters(const dash::core::DynamicsParameters& params)
{
    params_ = params;
    targetLufsSlider_.setValue(params.targetLufs, juce::dontSendNotification);
    ratioSlider_.setValue(params.ratio, juce::dontSendNotification);
    attackSlider_.setValue(params.attackMs, juce::dontSendNotification);
    releaseSlider_.setValue(params.releaseMs, juce::dontSendNotification);
    upwardSlider_.setValue(params.upwardAmount, juce::dontSendNotification);
    downwardSlider_.setValue(params.downwardAmount, juce::dontSendNotification);
    makeupGainSlider_.setValue(params.makeupGainDb, juce::dontSendNotification);
    bypassToggle_.setToggleState(params.bypass, juce::dontSendNotification);
    repaint();
}

dash::core::DynamicsParameters DynamicsPanel::getParameters() const
{
    dash::core::DynamicsParameters p;
    p.targetLufs     = static_cast<float>(targetLufsSlider_.getValue());
    p.ratio          = static_cast<float>(ratioSlider_.getValue());
    p.attackMs       = static_cast<float>(attackSlider_.getValue());
    p.releaseMs      = static_cast<float>(releaseSlider_.getValue());
    p.upwardAmount   = static_cast<float>(upwardSlider_.getValue());
    p.downwardAmount = static_cast<float>(downwardSlider_.getValue());
    p.makeupGainDb   = static_cast<float>(makeupGainSlider_.getValue());
    p.bypass         = bypassToggle_.getToggleState();
    return p;
}

} // namespace dash::ui
