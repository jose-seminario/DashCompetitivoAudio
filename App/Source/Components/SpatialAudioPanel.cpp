#include "SpatialAudioPanel.h"
#include "../Theme/DashLookAndFeel.h"

#include <cmath>

namespace dash::ui
{

SpatialAudioPanel::SpatialAudioPanel()
{
    titleLabel_.setText(juce::String::fromUTF8(u8"MOTOR ESPACIAL 3D, HRTF Y RENDERIZADOR BINAURAL (HASTA 16 CANALES)"), juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(14.0f).withStyle("Bold")));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCoralLight);
    addAndMakeVisible(titleLabel_);

    bypassToggle_.setButtonText(juce::String::fromUTF8(u8"Bypass Espacial"));
    bypassToggle_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    bypassToggle_.onClick = [this] {
        params_.bypass = bypassToggle_.getToggleState();
        if (onParametersChanged) onParametersChanged(params_);
        repaint();
    };
    addAndMakeVisible(bypassToggle_);

    hrtfModelLabel_.setText(juce::String::fromUTF8(u8"Modelo de Síntesis HRTF / HRIR:"), juce::dontSendNotification);
    hrtfModelLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f)));
    addAndMakeVisible(hrtfModelLabel_);

    hrtfModelSelector_.addItem(juce::String::fromUTF8(u8"Difracción Analítica Esférica Woodworth (Cero Latencia)"), 1);
    hrtfModelSelector_.addItem(juce::String::fromUTF8(u8"Adaptador de Respuestas al Impulso HeSuVi (WAV Multicanal)"), 2);
    hrtfModelSelector_.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(hrtfModelSelector_);

    // Ampliación del rango hasta 3.0x para escenario y separación frente/atrás
    initKnob(widthSlider_, widthLabel_, juce::String::fromUTF8(u8"Anchura de Escenario"), 0.5, 3.0, 1.0, "x");
    initKnob(distanceSlider_, distanceLabel_, juce::String::fromUTF8(u8"Distancia Acústica"), 0.5, 4.0, 1.4, " m");
    initKnob(frontBackSlider_, frontBackLabel_, juce::String::fromUTF8(u8"Separación Frente/Atrás"), 0.0, 3.0, 1.0, "x");
    initKnob(elevationSlider_, elevationLabel_, juce::String::fromUTF8(u8"Énfasis Altura (Techo)"), -6.0, 6.0, 0.0, " dB");

    // Control Deslizante de Sangrado Espacial (Crossfeed con LowPass 700 Hz y 0.3 ms ITD)
    crossfeedLabel_.setText(juce::String::fromUTF8(u8"Sangrado Espacial (Crossfeed):"), juce::dontSendNotification);
    crossfeedLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f).withStyle("Bold")));
    crossfeedLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(crossfeedLabel_);

    crossfeedSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    crossfeedSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 55, 18);
    crossfeedSlider_.setRange(0.0, 50.0, 1.0);
    crossfeedSlider_.setValue(25.0, juce::dontSendNotification);
    crossfeedSlider_.setTextValueSuffix("%");
    crossfeedSlider_.setColour(juce::Slider::trackColourId, DashLookAndFeel::kPillTrackDark);
    crossfeedSlider_.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    crossfeedSlider_.onValueChange = [this] {
        params_ = getParameters();
        if (onParametersChanged) onParametersChanged(params_);
        repaint();
    };
    addAndMakeVisible(crossfeedSlider_);

    // Control Deslizante de Mezcla Espacial (HRTF vs Estéreo)
    spatialMixLabel_.setText(juce::String::fromUTF8(u8"Mezcla Espacial (HRTF vs Estéreo):"), juce::dontSendNotification);
    spatialMixLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f).withStyle("Bold")));
    spatialMixLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(spatialMixLabel_);

    spatialMixSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    spatialMixSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 55, 18);
    spatialMixSlider_.setRange(0.0, 1.0, 0.01);
    spatialMixSlider_.setValue(0.3, juce::dontSendNotification);
    spatialMixSlider_.setColour(juce::Slider::trackColourId, DashLookAndFeel::kPillTrackDark);
    spatialMixSlider_.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    spatialMixSlider_.onValueChange = [this] {
        params_ = getParameters();
        DBG(juce::String::fromUTF8(u8"[SpatialAudioPanel] Slider Mezcla Espacial modificado: ") + juce::String(params_.spatialMix, 2));
        if (onParametersChanged) onParametersChanged(params_);
        repaint();
    };
    addAndMakeVisible(spatialMixSlider_);

    spatialMixHrtfHintLabel_.setText(juce::String::fromUTF8(u8"100% HRTF (Más 3D)"), juce::dontSendNotification);
    spatialMixHrtfHintLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(10.5f)));
    spatialMixHrtfHintLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(spatialMixHrtfHintLabel_);

    spatialMixStereoHintLabel_.setText(juce::String::fromUTF8(u8"100% Estéreo (Más Cuerpo)"), juce::dontSendNotification);
    spatialMixStereoHintLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(10.5f)));
    spatialMixStereoHintLabel_.setJustificationType(juce::Justification::topRight);
    spatialMixStereoHintLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(spatialMixStereoHintLabel_);

    lfeBypassNoteLabel_.setText(juce::String::fromUTF8(u8"Arquitectura de Fase: El canal LFE (subgraves < 120 Hz) elude automáticamente la convolución HRTF para erradicar cancelaciones destructivas (comb-filtering) en auriculares."), juce::dontSendNotification);
    lfeBypassNoteLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f)));
    lfeBypassNoteLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(lfeBypassNoteLabel_);
}

void SpatialAudioPanel::initKnob(
    juce::Slider& s, juce::Label& l, const juce::String& text,
    double min, double max, double def, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    s.setRange(min, max, 0.05);
    s.setValue(def, juce::dontSendNotification);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    s.onValueChange = [this] {
        params_ = getParameters();
        if (onParametersChanged) onParametersChanged(params_);
        repaint();
    };
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(l);
}

void SpatialAudioPanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    const float cardW = (float)getWidth() - 32.0f;

    // Tarjeta izquierda (Parámetros)
    const auto leftCard = juce::Rectangle<float>(16, 44, 460, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(leftCard, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(leftCard, 8.0f, 1.2f);

    // Tarjeta derecha (Diagrama 3D de Altavoces)
    const auto rightCard = juce::Rectangle<float>(490, 44, cardW - 474, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(rightCard, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(rightCard, 8.0f, 1.2f);

    g.setColour(DashLookAndFeel::kTextMain);
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f).withStyle("Bold")));
    g.drawText(juce::String::fromUTF8(u8"CAMPO ACÚSTICO 3D (DISPOSICIÓN 7.1 EN AURICULARES)"), 506, 56, (int)rightCard.getWidth() - 32, 20, juce::Justification::centredLeft);

    drawTopDownSpeakerLayout(g, rightCard.reduced(24.0f).withTrimmedTop(36.0f));
}

void SpatialAudioPanel::drawTopDownSpeakerLayout(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    const float centreX = area.getCentreX();
    const float centreY = area.getCentreY();
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.38f;

    // Círculos concéntricos de distancia
    g.setColour(DashLookAndFeel::kBorderSubtle.withAlpha(0.6f));
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    g.drawEllipse(centreX - radius * 0.6f, centreY - radius * 0.6f, radius * 1.2f, radius * 1.2f, 0.8f);

    // Cabeza del oyente central
    g.setColour(DashLookAndFeel::kBgCardElevated);
    g.fillEllipse(centreX - 18.0f, centreY - 18.0f, 36.0f, 36.0f);
    g.setColour(DashLookAndFeel::kAccentBlue);
    g.drawEllipse(centreX - 18.0f, centreY - 18.0f, 36.0f, 36.0f, 1.5f);

    // Nariz indicadora frontal
    juce::Path nose;
    nose.startNewSubPath(centreX - 4.0f, centreY - 18.0f);
    nose.lineTo(centreX, centreY - 26.0f);
    nose.lineTo(centreX + 4.0f, centreY - 18.0f);
    g.strokePath(nose, juce::PathStrokeType(1.5f));

    // Posiciones de los 8 altavoces en 7.1 (Ángulos acimutal estándar)
    struct Spk { const char* label; float deg; bool isLfe; };
    static const std::array<Spk, 8> spks = {{
        { "C",    0.0f, false },
        { "FL", -30.0f, false },
        { "FR",  30.0f, false },
        { "SL", -90.0f, false },
        { "SR",  90.0f, false },
        { "BL",-150.0f, false },
        { "BR", 150.0f, false },
        { "LFE",  0.0f, true  }
    }};

    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(10.0f).withStyle("Bold")));
    for (const auto& spk : spks)
    {
        float r = radius;
        if (spk.isLfe)
            r = radius * 0.45f;

        const float rad = juce::degreesToRadians(spk.deg - 90.0f);
        const float sx = centreX + r * std::cos(rad);
        const float sy = centreY + r * std::sin(rad);

        g.setColour(spk.isLfe ? DashLookAndFeel::kWarningAmber : DashLookAndFeel::kAccentCyan);
        g.fillEllipse(sx - 10.0f, sy - 10.0f, 20.0f, 20.0f);
        g.setColour(juce::Colours::black);
        g.drawText(spk.label, (int)sx - 10, (int)sy - 10, 20, 20, juce::Justification::centred);
    }
}

void SpatialAudioPanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);
    bypassToggle_.setBounds(getWidth() - 200, 12, 180, 24);

    const int startX = 36;
    hrtfModelLabel_.setBounds(startX, 60, 420, 20);
    hrtfModelSelector_.setBounds(startX, 84, 420, 30);

    const int knobW = 120;
    const int knobH = 96;

    // Fila 1 de Perillas
    const int row1Y = 126;
    widthLabel_.setBounds(startX, row1Y, knobW, 16);
    widthSlider_.setBounds(startX, row1Y + 16, knobW, knobH);

    distanceLabel_.setBounds(startX + 140, row1Y, knobW, 16);
    distanceSlider_.setBounds(startX + 140, row1Y + 16, knobW, knobH);

    // Fila 2 de Perillas
    const int row2Y = 236;
    frontBackLabel_.setBounds(startX, row2Y, knobW, 16);
    frontBackSlider_.setBounds(startX, row2Y + 16, knobW, knobH);

    elevationLabel_.setBounds(startX + 140, row2Y, knobW, 16);
    elevationSlider_.setBounds(startX + 140, row2Y + 16, knobW, knobH);

    // Fila 3: Control Deslizante de Sangrado Espacial (Crossfeed)
    const int row3Y = 345;
    crossfeedLabel_.setBounds(startX, row3Y, 420, 18);
    crossfeedSlider_.setBounds(startX, row3Y + 20, 420, 24);

    // Fila 4: Control Deslizante de Mezcla Espacial
    const int row4Y = 398;
    spatialMixLabel_.setBounds(startX, row4Y, 420, 18);
    spatialMixSlider_.setBounds(startX, row4Y + 20, 420, 24);
    spatialMixHrtfHintLabel_.setBounds(startX, row4Y + 46, 190, 16);
    spatialMixStereoHintLabel_.setBounds(startX + 230, row4Y + 46, 190, 16);

    lfeBypassNoteLabel_.setBounds(startX, getHeight() - 75, 430, 55);
}

void SpatialAudioPanel::setParameters(const dash::spatial::SpatialProcessorParameters& params)
{
    params_ = params;
    widthSlider_.setValue(params.soundstageWidth, juce::dontSendNotification);
    distanceSlider_.setValue(params.apparentDistanceM, juce::dontSendNotification);
    frontBackSlider_.setValue(params.frontBackSeparation, juce::dontSendNotification);
    elevationSlider_.setValue(params.elevationEmphasis, juce::dontSendNotification);
    crossfeedSlider_.setValue(params.crossfeedAmount * 100.0f, juce::dontSendNotification);
    spatialMixSlider_.setValue(params.spatialMix, juce::dontSendNotification);
    bypassToggle_.setToggleState(params.bypass, juce::dontSendNotification);
    repaint();
}

dash::spatial::SpatialProcessorParameters SpatialAudioPanel::getParameters() const
{
    dash::spatial::SpatialProcessorParameters p;
    p.soundstageWidth     = static_cast<float>(widthSlider_.getValue());
    p.apparentDistanceM   = static_cast<float>(distanceSlider_.getValue());
    p.frontBackSeparation = static_cast<float>(frontBackSlider_.getValue());
    p.elevationEmphasis   = static_cast<float>(elevationSlider_.getValue());
    p.crossfeedAmount     = static_cast<float>(crossfeedSlider_.getValue() / 100.0f);
    p.spatialMix          = static_cast<float>(spatialMixSlider_.getValue());
    p.bypass              = bypassToggle_.getToggleState();
    return p;
}

} // namespace dash::ui
