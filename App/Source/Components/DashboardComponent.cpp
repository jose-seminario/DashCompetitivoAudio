#include "DashboardComponent.h"
#include "../Theme/DashLookAndFeel.h"
#include <cmath>

namespace dash::ui
{

DashboardComponent::DashboardComponent()
{
    // 1. Selector de Modo (Easy / Advanced)
    easyModeBtn_.setButtonText(juce::String::fromUTF8(u8"MODO JUGADOR"));
    easyModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentCoral);
    easyModeBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kPillWhite);
    easyModeBtn_.onClick = [this] {
        currentMode_ = DisplayMode::Easy;
        easyModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentCoral);
        easyModeBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kPillWhite);
        advancedModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
        advancedModeBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kTextMuted);
        if (onDisplayModeChanged) onDisplayModeChanged(DisplayMode::Easy);
        resized();
        repaint();
    };
    addAndMakeVisible(easyModeBtn_);

    advancedModeBtn_.setButtonText(juce::String::fromUTF8(u8"MODO INGENIERO"));
    advancedModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    advancedModeBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kTextMuted);
    advancedModeBtn_.onClick = [this] {
        currentMode_ = DisplayMode::Advanced;
        advancedModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentCoral);
        advancedModeBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kPillWhite);
        easyModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
        easyModeBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kTextMuted);
        if (onDisplayModeChanged) onDisplayModeChanged(DisplayMode::Advanced);
        resized();
        repaint();
    };
    addAndMakeVisible(advancedModeBtn_);

    // 2. Botones Maestros de Control del Motor
    activateBtn_.setButtonText(juce::String::fromUTF8(u8"ACTIVAR AUDIO TÁCTICO"));
    activateBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kPillWhite);
    activateBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kAccentCoral);
    activateBtn_.onClick = [this] { if (onActivateClicked) onActivateClicked(); };
    addAndMakeVisible(activateBtn_);

    deactivateBtn_.setButtonText(juce::String::fromUTF8(u8"BYPASS / NATIVO"));
    deactivateBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    deactivateBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kTextMuted);
    deactivateBtn_.onClick = [this] { if (onDeactivateClicked) onDeactivateClicked(); };
    addAndMakeVisible(deactivateBtn_);

    // 3. Tutorial de Primer Uso y Autoconfiguración
    tutorialTitleLabel_.setText(juce::String::fromUTF8(u8"GUÍA RÁPIDA DE PRIMER USO"), juce::dontSendNotification);
    tutorialTitleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f).withStyle("Bold")));
    tutorialTitleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCoralLight);
    addAndMakeVisible(tutorialTitleLabel_);

    step1Label_.setText(juce::String::fromUTF8(u8"Paso 1: Salida en Windows: 'CABLE Input'."), juce::dontSendNotification);
    step1Label_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f)));
    step1Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(step1Label_);

    autoConfigBtn_.setButtonText(juce::String::fromUTF8(u8"Autoconfigurar Audio (VB-Cable)"));
    autoConfigBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentCoralLight);
    autoConfigBtn_.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kPillWhite);
    autoConfigBtn_.onClick = [this] {
        if (onAutoConfigClicked)
            onAutoConfigClicked();
    };
    addAndMakeVisible(autoConfigBtn_);

    step2Label_.setText(juce::String::fromUTF8(u8"Paso 2: Haz clic en 'ACTIVAR AUDIO TÁCTICO'."), juce::dontSendNotification);
    step2Label_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f)));
    step2Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(step2Label_);

    step3Label_.setText(juce::String::fromUTF8(u8"Paso 3: ¡Listo para competir!"), juce::dontSendNotification);
    step3Label_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.0f).withStyle("Bold")));
    step3Label_.setColour(juce::Label::textColourId, DashLookAndFeel::kSuccessEmerald);
    addAndMakeVisible(step3Label_);

    dismissTutorialBtn_.setButtonText(juce::String::fromUTF8(u8"Ocultar Guía"));
    dismissTutorialBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    dismissTutorialBtn_.onClick = [this] { setTutorialVisible(false); };
    addAndMakeVisible(dismissTutorialBtn_);

    // 4. Cadena visual de 9 nodos interactivos
    chainNodes_ = {
        { "INPUT",     "0.0 dBFS", false, {} },
        { "HEADROOM",  "-6.0 dB",  false, {} },
        { "DYNAMICS",  "BS.1770",  false, {} },
        { "TRANSIENT", "Disparos", false, {} },
        { "AMBIENCE",  "Ruido",    false, {} },
        { "SPATIAL",   "HRTF 3D",  false, {} },
        { "EQUALIZER", "ISO 31",   false, {} },
        { "LIMITER",   "-0.5 dB",  false, {} },
        { "OUTPUT",    "Binaural", false, {} }
    };

    // 5. Controles Tácticos Rápidos: Rack de 5 Faders Verticales
    quickTitleLabel_.setText(juce::String::fromUTF8(u8"RACK DE CONTROL TÁCTICO (FADERS DE PRECISIÓN)"), juce::dontSendNotification);
    quickTitleLabel_.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f).withStyle("Bold")));
    quickTitleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(quickTitleLabel_);

    quickProfileSelector_.addItem("Warzone Competitive Default", 1);
    quickProfileSelector_.addItem("Warzone Balanced Default", 2);
    quickProfileSelector_.addItem("Warzone Immersive Spatial", 3);
    quickProfileSelector_.addItem("Footsteps Extreme Focus (Comunitario)", 4);
    quickProfileSelector_.setSelectedId(1, juce::dontSendNotification);
    quickProfileSelector_.onChange = [this] {
        if (onQuickProfileSelected)
            onQuickProfileSelected(quickProfileSelector_.getText());
    };
    addAndMakeVisible(quickProfileSelector_);

    initVerticalFader(gunSlider_,        gunLabel_,        juce::String::fromUTF8(u8"DISPAROS"),    0.0, 1.0, 0.45, "");
    initVerticalFader(stepsSlider_,      stepsLabel_,      juce::String::fromUTF8(u8"PASOS"),       -6.0, 12.0, 2.0, " dB");
    initVerticalFader(ambSlider_,        ambLabel_,        juce::String::fromUTF8(u8"RUIDO"),       0.0, 1.0, 1.0, "");
    initVerticalFader(makeupSlider_,     makeupLabel_,     juce::String::fromUTF8(u8"VOLUMEN"),     0.0, 12.0, 6.0, " dB");
    initVerticalFader(spatialMixSlider_, spatialMixLabel_, juce::String::fromUTF8(u8"3D MEZCLA"),   0.0, 1.0, 0.20, "");

    auto notifyQuickControls = [this] {
        if (onQuickControlsChanged)
        {
            float gun, steps, amb, makeup;
            bool spatial;
            getQuickControlsValues(gun, steps, amb, makeup, spatial);
            onQuickControlsChanged(gun, steps, amb, makeup, spatial);
        }
    };
    gunSlider_.onValueChange        = notifyQuickControls;
    stepsSlider_.onValueChange      = notifyQuickControls;
    ambSlider_.onValueChange        = notifyQuickControls;
    makeupSlider_.onValueChange     = notifyQuickControls;
    spatialMixSlider_.onValueChange = notifyQuickControls;

    spatialToggle_.setButtonText(juce::String::fromUTF8(u8"Audio Espacial 3D (Binaural HRTF)"));
    spatialToggle_.setToggleState(true, juce::dontSendNotification);
    spatialToggle_.onClick = notifyQuickControls;
    addAndMakeVisible(spatialToggle_);
}

void DashboardComponent::initVerticalFader(
    juce::Slider& s, juce::Label& l, const juce::String& text,
    double min, double max, double def, const juce::String& suffix)
{
    // Fader Vertical idéntico a la referencia
    s.setSliderStyle(juce::Slider::LinearVertical);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 20);
    s.setRange(min, max, (max <= 1.0) ? 0.01 : 0.1);
    s.setValue(def, juce::dontSendNotification);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::trackColourId, DashLookAndFeel::kPillTrackDark);
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f).withStyle("Bold")));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(l);
}

void DashboardComponent::setEngineRunning(bool running)
{
    isRunning_ = running;
    activateBtn_.setButtonText(running ? juce::String::fromUTF8(u8"MOTOR TÁCTICO ACTIVO") : juce::String::fromUTF8(u8"ACTIVAR AUDIO TÁCTICO"));
    activateBtn_.setColour(juce::TextButton::buttonColourId, running ? DashLookAndFeel::kSuccessEmerald : DashLookAndFeel::kPillWhite);
    activateBtn_.setColour(juce::TextButton::textColourOffId, running ? DashLookAndFeel::kPillWhite : DashLookAndFeel::kAccentCoral);
    repaint();
}

void DashboardComponent::setDeviceName(const juce::String& name)
{
    deviceName_ = name;
    repaint();
}

void DashboardComponent::setFormatInfo(double sampleRate, const juce::String& layout)
{
    formatText_ = juce::String(static_cast<int>(sampleRate)) + " Hz | " + layout;
    repaint();
}

void DashboardComponent::setLatencyMs(float latencyMs)
{
    latencyMs_ = latencyMs;
    repaint();
}

void DashboardComponent::setCpuPercent(float cpuPercent)
{
    cpuPercent_ = cpuPercent;
    repaint();
}

void DashboardComponent::setTutorialVisible(bool visible)
{
    showTutorial_ = visible;
    tutorialTitleLabel_.setVisible(visible);
    step1Label_.setVisible(visible);
    step2Label_.setVisible(visible);
    step3Label_.setVisible(visible);
    autoConfigBtn_.setVisible(visible);
    dismissTutorialBtn_.setVisible(visible);
    resized();
    repaint();
}

void DashboardComponent::updateQuickControls(float gunReductionDb, float footstepsDb, float ambienceReductionDb, float makeupGainDb, bool spatialEnabled)
{
    gunSlider_.setValue(gunReductionDb, juce::dontSendNotification);
    stepsSlider_.setValue(footstepsDb, juce::dontSendNotification);
    ambSlider_.setValue(ambienceReductionDb, juce::dontSendNotification);
    makeupSlider_.setValue(makeupGainDb, juce::dontSendNotification);
    spatialToggle_.setToggleState(spatialEnabled, juce::dontSendNotification);
}

void DashboardComponent::getQuickControlsValues(float& outGunReductionDb, float& outFootstepsDb, float& outAmbienceReductionDb, float& outMakeupGainDb, bool& outSpatialEnabled) const
{
    outGunReductionDb     = static_cast<float>(gunSlider_.getValue());
    outFootstepsDb        = static_cast<float>(stepsSlider_.getValue());
    outAmbienceReductionDb= static_cast<float>(ambSlider_.getValue());
    outMakeupGainDb       = static_cast<float>(makeupSlider_.getValue());
    outSpatialEnabled     = spatialToggle_.getToggleState();
}

void DashboardComponent::setNodeBypassState(int nodeIndex, bool isBypassed)
{
    if (nodeIndex >= 0 && nodeIndex < static_cast<int>(chainNodes_.size()))
    {
        if (chainNodes_[static_cast<size_t>(nodeIndex)].bypassed != isBypassed)
        {
            chainNodes_[static_cast<size_t>(nodeIndex)].bypassed = isBypassed;
            repaint();
        }
    }
}

void DashboardComponent::mouseDown(const juce::MouseEvent& e)
{
    for (size_t i = 0; i < chainNodes_.size(); ++i)
    {
        if (chainNodes_[i].bounds.contains(e.getPosition()))
        {
            if (onChainNodeClicked)
                onChainNodeClicked(static_cast<int>(i));
            break;
        }
    }
}

void DashboardComponent::drawHeroCard(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    // Tarjeta Hero Coral Neón (Idéntica a la tarjeta destacada de la referencia)
    juce::ColourGradient heroGrad(
        DashLookAndFeel::kAccentCoralLight, area.getX(), area.getY(),
        DashLookAndFeel::kAccentCoral, area.getRight(), area.getBottom(), false);
    g.setGradientFill(heroGrad);
    g.fillRoundedRectangle(area, 18.0f);

    // Borde de resplandor sutil
    g.setColour(juce::Colours::white.withAlpha(0.20f));
    g.drawRoundedRectangle(area.reduced(0.5f), 18.0f, 1.2f);

    // Silueta / Emblema Táctico en el cuadrante derecho
    const float emblemSize = 72.0f;
    const auto emblemArea = juce::Rectangle<float>(area.getRight() - emblemSize - 18.0f, area.getCentreY() - emblemSize * 0.5f, emblemSize, emblemSize);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.fillEllipse(emblemArea);

    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.drawEllipse(emblemArea.reduced(8.0f), 2.0f);
    g.fillEllipse(emblemArea.getCentreX() - 8.0f, emblemArea.getCentreY() - 8.0f, 16.0f, 16.0f);

    // Título y Subtítulo en blanco puro
    g.setColour(DashLookAndFeel::kPillWhite);
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(17.0f).withStyle("Bold")));
    g.drawText("DASH COMPETITIVO AUDIO", (int)area.getX() + 20, (int)area.getY() + 16, (int)area.getWidth() - (int)emblemSize - 40, 22, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(12.5f)));
    g.setColour(DashLookAndFeel::kPillWhite.withAlpha(0.85f));
    g.drawText("Perfil Activo: " + quickProfileSelector_.getText(), (int)area.getX() + 20, (int)area.getY() + 40, (int)area.getWidth() - (int)emblemSize - 40, 18, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f)));
    g.setColour(DashLookAndFeel::kPillWhite.withAlpha(0.70f));
    g.drawText(deviceName_ + " | " + formatText_, (int)area.getX() + 20, (int)area.getY() + 60, (int)area.getWidth() - (int)emblemSize - 40, 16, juce::Justification::centredLeft);
}

void DashboardComponent::drawWaveformCard(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    // Tarjeta Waveform oscura (Inspirada directamente en la tarjeta de onda de la imagen)
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(area, 14.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(area, 14.0f, 1.0f);

    // Título de la tarjeta
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.0f).withStyle("Bold")));
    g.setColour(DashLookAndFeel::kTextMuted);
    g.drawText("ACTIVIDAD ESPECTRAL EN TIEMPO REAL", (int)area.getX() + 14, (int)area.getY() + 10, (int)area.getWidth() - 28, 14, juce::Justification::centredLeft);

    // Dibujo de la forma de onda de audio en líneas verticales Coral
    const float midY = area.getY() + area.getHeight() * 0.52f;
    const float waveW = area.getWidth() - 28.0f;
    const float startX = area.getX() + 14.0f;
    const int numBars = 36;
    const float barStep = waveW / static_cast<float>(numBars);

    for (int i = 0; i < numBars; ++i)
    {
        const float x = startX + static_cast<float>(i) * barStep;
        // Altura sintética de onda reactiva con modulación sinusoidal realista
        const float phase = static_cast<float>(i) * 0.28f;
        const float envelope = std::sin(static_cast<float>(i) / static_cast<float>(numBars) * 3.14159f);
        const float heightAmp = (std::sin(phase) * 0.5f + 0.5f) * envelope * (area.getHeight() * 0.34f) + 4.0f;

        g.setColour(DashLookAndFeel::kAccentCoralLight);
        g.drawLine(x, midY - heightAmp, x, midY + heightAmp, 2.2f);
    }

    // Badges inferiores: Latencia y CPU en tiempo real
    const float tagY = area.getBottom() - 26.0f;
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(10.5f).withStyle("Bold")));
    g.setColour(DashLookAndFeel::kAccentCyan);
    juce::String latInfo = (latencyMs_ > 0.0f) ? "Latencia: " + juce::String(latencyMs_, 1) + " ms" : "Latencia: --";
    g.drawText(latInfo, (int)area.getX() + 14, (int)tagY, 130, 16, juce::Justification::centredLeft);

    g.setColour(DashLookAndFeel::kAccentCoralLight);
    juce::String cpuInfo = "DSP CPU: " + juce::String(cpuPercent_, 1) + "%";
    g.drawText(cpuInfo, (int)area.getRight() - 120, (int)tagY, 106, 16, juce::Justification::centredRight);
}

void DashboardComponent::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    if (getWidth() <= 50 || getHeight() <= 50)
        return;

    // 1. Selector de Perfil y Modo (Fila superior)
    // 2. Hero Card (Lado izquierdo) y Waveform Card (Lado derecho)
    const float topY = 48.0f;
    const float heroW = (static_cast<float>(getWidth()) - 42.0f) * 0.58f;
    const float waveW = (static_cast<float>(getWidth()) - 42.0f) * 0.42f;
    const float topCardH = 124.0f;

    drawHeroCard(g, juce::Rectangle<float>(14.0f, topY, heroW, topCardH));
    drawWaveformCard(g, juce::Rectangle<float>(14.0f + heroW + 14.0f, topY, waveW, topCardH));

    float curCardY = topY + topCardH + 12.0f;

    // 3. Tarjeta del Tutorial de Primer Uso (si está activo)
    if (showTutorial_)
    {
        const auto tutArea = juce::Rectangle<float>(14.0f, curCardY, (float)getWidth() - 28.0f, 62.0f);
        g.setColour(DashLookAndFeel::kBgCard);
        g.fillRoundedRectangle(tutArea, 12.0f);
        g.setColour(DashLookAndFeel::kAccentCoralLight.withAlpha(0.5f));
        g.drawRoundedRectangle(tutArea, 12.0f, 1.0f);

        curCardY += 72.0f;
    }

    // 4. Cadena de Procesamiento (9 Nodos Tácticos en Cápsula)
    const auto chainArea = juce::Rectangle<float>(14.0f, curCardY, (float)getWidth() - 28.0f, 74.0f);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(chainArea, 12.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(chainArea, 12.0f, 1.0f);

    // Dibujar nodos de la cadena
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(10.5f).withStyle("Bold")));
    for (size_t i = 0; i < chainNodes_.size(); ++i)
    {
        const auto& node = chainNodes_[i];
        if (node.bounds.isEmpty())
            continue;

        const auto b = node.bounds.toFloat().reduced(1.5f);
        g.setColour(node.bypassed ? DashLookAndFeel::kBgCardElevated.darker(0.2f) : DashLookAndFeel::kBgCardElevated);
        g.fillRoundedRectangle(b, 8.0f);

        g.setColour(node.bypassed ? DashLookAndFeel::kTextMuted : DashLookAndFeel::kAccentCoralLight);
        g.drawRoundedRectangle(b, 8.0f, 1.0f);

        g.setColour(node.bypassed ? DashLookAndFeel::kTextMuted : DashLookAndFeel::kTextMain);
        g.drawText(node.name, (int)b.getX(), (int)b.getY() + 4, (int)b.getWidth(), 14, juce::Justification::centred);

        g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(9.5f)));
        g.setColour(node.bypassed ? DashLookAndFeel::kDangerCrimson : DashLookAndFeel::kAccentCoral);
        g.drawText(node.bypassed ? "BYPASS" : node.info, (int)b.getX(), (int)b.getY() + 18, (int)b.getWidth(), 14, juce::Justification::centred);

        // Flecha conectora hacia el siguiente nodo
        if (i + 1 < chainNodes_.size())
        {
            const float arrowX = b.getRight() + 2.0f;
            const float arrowY = b.getCentreY();
            g.setColour(DashLookAndFeel::kAccentCoralLight.withAlpha(0.6f));
            g.drawLine(arrowX, arrowY, arrowX + 5.0f, arrowY, 1.2f);
        }
    }

    curCardY += 84.0f;

    // 5. Contenedor de Faders Tácticos (Easy Mode)
    if (currentMode_ == DisplayMode::Easy)
    {
        const auto fadersArea = juce::Rectangle<float>(14.0f, curCardY, (float)getWidth() - 28.0f, (float)getHeight() - curCardY - 10.0f);
        g.setColour(DashLookAndFeel::kBgCard);
        g.fillRoundedRectangle(fadersArea, 14.0f);
        g.setColour(DashLookAndFeel::kBorderSubtle);
        g.drawRoundedRectangle(fadersArea, 14.0f, 1.0f);
    }
}

void DashboardComponent::resized()
{
    if (getWidth() <= 50 || getHeight() <= 50)
        return;

    // Fila superior: Selector de Modo e Input de Perfil
    easyModeBtn_.setBounds(14, 10, 140, 28);
    advancedModeBtn_.setBounds(162, 10, 150, 28);
    quickProfileSelector_.setBounds(getWidth() - 280, 10, 266, 28);

    // Botones Maestros del Motor (Ubicados en la Hero Card)
    const float topY = 48.0f;
    const float heroW = (static_cast<float>(getWidth()) - 42.0f) * 0.58f;
    activateBtn_.setBounds(static_cast<int>(heroW - 190.0f), static_cast<int>(topY + 84.0f), 190, 28);
    deactivateBtn_.setBounds(static_cast<int>(heroW - 320.0f), static_cast<int>(topY + 84.0f), 120, 28);

    int curY = static_cast<int>(topY + 124.0f + 12.0f);

    // Tutorial de Primer Uso
    if (showTutorial_)
    {
        tutorialTitleLabel_.setBounds(28, curY + 6, 220, 16);
        dismissTutorialBtn_.setBounds(getWidth() - 118, curY + 6, 90, 18);

        step1Label_.setBounds(28, curY + 24, 320, 16);
        autoConfigBtn_.setBounds(350, curY + 22, 210, 22);

        step2Label_.setBounds(28, curY + 40, 260, 16);
        step3Label_.setBounds(300, curY + 40, 160, 16);

        curY += 72;
    }

    // Cadena Visual (9 nodos)
    const int chainStartX = 20;
    const int chainY = curY + 24;
    const int availableW = getWidth() - 40;
    const int nodeW = (availableW - (static_cast<int>(chainNodes_.size()) - 1) * 8) / static_cast<int>(chainNodes_.size());
    const int nodeH = 36;

    for (size_t i = 0; i < chainNodes_.size(); ++i)
    {
        chainNodes_[i].bounds = juce::Rectangle<int>(chainStartX + static_cast<int>(i) * (nodeW + 8), chainY, nodeW, nodeH);
    }

    curY += 84;

    // Faders Tácticos en Cápsula (Easy Mode)
    const bool isEasy = (currentMode_ == DisplayMode::Easy);
    quickTitleLabel_.setVisible(isEasy);
    gunSlider_.setVisible(isEasy);
    gunLabel_.setVisible(isEasy);
    stepsSlider_.setVisible(isEasy);
    stepsLabel_.setVisible(isEasy);
    ambSlider_.setVisible(isEasy);
    ambLabel_.setVisible(isEasy);
    makeupSlider_.setVisible(isEasy);
    makeupLabel_.setVisible(isEasy);
    spatialMixSlider_.setVisible(isEasy);
    spatialMixLabel_.setVisible(isEasy);
    spatialToggle_.setVisible(isEasy);

    if (isEasy)
    {
        quickTitleLabel_.setBounds(28, curY + 10, 380, 18);
        spatialToggle_.setBounds(getWidth() - 270, curY + 8, 250, 24);

        const int faderY = curY + 32;
        const int faderH = getHeight() - faderY - 18;
        const int numFaders = 5;
        const int availableFaderW = getWidth() - 56;
        const int slotW = availableFaderW / numFaders;

        // 1. Disparos
        gunLabel_.setBounds(28, faderY, slotW, 16);
        gunSlider_.setBounds(28, faderY + 18, slotW, faderH - 22);

        // 2. Pasos
        stepsLabel_.setBounds(28 + slotW, faderY, slotW, 16);
        stepsSlider_.setBounds(28 + slotW, faderY + 18, slotW, faderH - 22);

        // 3. Ruido
        ambLabel_.setBounds(28 + slotW * 2, faderY, slotW, 16);
        ambSlider_.setBounds(28 + slotW * 2, faderY + 18, slotW, faderH - 22);

        // 4. Volumen
        makeupLabel_.setBounds(28 + slotW * 3, faderY, slotW, 16);
        makeupSlider_.setBounds(28 + slotW * 3, faderY + 18, slotW, faderH - 22);

        // 5. 3D Mezcla
        spatialMixLabel_.setBounds(28 + slotW * 4, faderY, slotW, 16);
        spatialMixSlider_.setBounds(28 + slotW * 4, faderY + 18, slotW, faderH - 22);
    }
}

} // namespace dash::ui
