#include "DashboardComponent.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

DashboardComponent::DashboardComponent()
{
    // 1. Selector Easy Mode / Advanced Mode
    easyModeBtn_.setButtonText("EASY MODE (JUGADOR)");
    easyModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
    easyModeBtn_.onClick = [this] {
        currentMode_ = DisplayMode::Easy;
        easyModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
        advancedModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
        if (onDisplayModeChanged) onDisplayModeChanged(DisplayMode::Easy);
        resized();
        repaint();
    };
    addAndMakeVisible(easyModeBtn_);

    advancedModeBtn_.setButtonText("ADVANCED MODE (INGENIERO)");
    advancedModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    advancedModeBtn_.onClick = [this] {
        currentMode_ = DisplayMode::Advanced;
        advancedModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentBlue);
        easyModeBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
        if (onDisplayModeChanged) onDisplayModeChanged(DisplayMode::Advanced);
        resized();
        repaint();
    };
    addAndMakeVisible(advancedModeBtn_);

    // 2. Tarjeta de Estado
    engineCardTitle_.setText("ESTADO DEL MOTOR ACÚSTICO", juce::dontSendNotification);
    engineCardTitle_.setFont(juce::FontOptions(13.5f).withStyle("Bold"));
    engineCardTitle_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(engineCardTitle_);

    backendLabel_.setText("Backend: Equalizer APO / Direct Routing", juce::dontSendNotification);
    backendLabel_.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(backendLabel_);

    deviceLabel_.setText("Dispositivo: Altavoces Predeterminados de Windows", juce::dontSendNotification);
    deviceLabel_.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(deviceLabel_);

    formatLabel_.setText("Formato: 48000 Hz | 7.1 Surround (8 canales)", juce::dontSendNotification);
    formatLabel_.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(formatLabel_);

    latencyLabel_.setText("Latencia: 5.3 ms (256 samples)", juce::dontSendNotification);
    latencyLabel_.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(latencyLabel_);

    cpuLabel_.setText("Carga CPU DSP: < 1.8%", juce::dontSendNotification);
    cpuLabel_.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(cpuLabel_);

    activateBtn_.setButtonText("ACTIVAR MOTOR TÁCTICO");
    activateBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kSuccessEmerald);
    activateBtn_.onClick = [this] { if (onActivateClicked) onActivateClicked(); };
    addAndMakeVisible(activateBtn_);

    deactivateBtn_.setButtonText("RESTAURAR AUDIO ORIGINAL");
    deactivateBtn_.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
    deactivateBtn_.onClick = [this] { if (onDeactivateClicked) onDeactivateClicked(); };
    addAndMakeVisible(deactivateBtn_);

    // 3. Cadena visual de 8 nodos
    chainNodes_ = {
        { "INPUT", "0.0 dBFS", false, {} },
        { "HEADROOM", "-6.0 dB", false, {} },
        { "DYNAMICS", "BS.1770-4", false, {} },
        { "TRANSIENT", "Gun Crack", false, {} },
        { "AMBIENCE", "350-2.5k", false, {} },
        { "SPATIAL", "HRTF 3D", false, {} },
        { "EQUALIZER", "ISO 31 + PEQ", false, {} },
        { "LIMITER", "-0.5 dBFS", false, {} },
        { "OUTPUT", "Binaural Out", false, {} }
    };

    // 4. Controles Tácticos Rápidos
    quickTitleLabel_.setText("PERSONALIZACIÓN TÁCTICA RÁPIDA (EASY MODE)", juce::dontSendNotification);
    quickTitleLabel_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
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

    initTacticalSlider(gunSlider_, gunLabel_, "Supresión de Disparos", 0.0, 12.0, 4.0, " dB");
    initTacticalSlider(stepsSlider_, stepsLabel_, "Realce de Pasos", -3.0, 6.0, 2.0, " dB");
    initTacticalSlider(ambSlider_, ambLabel_, "Reducción Ambiental", 0.0, 12.0, 6.0, " dB");

    spatialToggle_.setButtonText("Audio Espacial Inmersivo (HRTF 3D)");
    spatialToggle_.setToggleState(true, juce::dontSendNotification);
    spatialToggle_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(spatialToggle_);
}

void DashboardComponent::initTacticalSlider(
    juce::Slider& s, juce::Label& l, const juce::String& text,
    double min, double max, double def, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 18);
    s.setRange(min, max, 0.1);
    s.setValue(def, juce::dontSendNotification);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::rotarySliderFillColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::FontOptions(11.5f));
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(l);
}

void DashboardComponent::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    // 1. Tarjeta del Motor
    const auto topCardArea = juce::Rectangle<float>(16, 52, (float)getWidth() - 32, 110);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(topCardArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(topCardArea, 8.0f, 1.2f);

    // 2. Tarjeta de Cadena de Procesamiento
    const auto chainArea = juce::Rectangle<float>(16, 172, (float)getWidth() - 32, 105);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(chainArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(chainArea, 8.0f, 1.2f);

    // Dibujar nodos de la cadena
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    for (size_t i = 0; i < chainNodes_.size(); ++i)
    {
        const auto& node = chainNodes_[i];
        if (node.bounds.isEmpty())
            continue;

        const auto b = node.bounds.toFloat().reduced(2.0f);
        g.setColour(node.bypassed ? DashLookAndFeel::kBgCardElevated.darker(0.2f) : DashLookAndFeel::kBgCardElevated);
        g.fillRoundedRectangle(b, 5.0f);

        g.setColour(node.bypassed ? DashLookAndFeel::kTextMuted : DashLookAndFeel::kBorderFocus);
        g.drawRoundedRectangle(b, 5.0f, 1.0f);

        g.setColour(node.bypassed ? DashLookAndFeel::kTextMuted : DashLookAndFeel::kTextMain);
        g.drawText(node.name, (int)b.getX(), (int)b.getY() + 8, (int)b.getWidth(), 14, juce::Justification::centred);

        g.setFont(juce::FontOptions(10.0f));
        g.setColour(node.bypassed ? DashLookAndFeel::kDangerCrimson : DashLookAndFeel::kAccentCyan);
        g.drawText(node.bypassed ? "BYPASS" : node.info, (int)b.getX(), (int)b.getY() + 26, (int)b.getWidth(), 14, juce::Justification::centred);

        // Flecha conectora hacia el siguiente nodo
        if (i + 1 < chainNodes_.size())
        {
            const float arrowX = b.getRight() + 4.0f;
            const float arrowY = b.getCentreY();
            g.setColour(DashLookAndFeel::kAccentCyan.withAlpha(0.7f));
            g.drawLine(arrowX, arrowY, arrowX + 8.0f, arrowY, 1.5f);
        }
    }

    // 3. Tarjeta de Controles Tácticos (Easy Mode)
    if (currentMode_ == DisplayMode::Easy)
    {
        const auto quickArea = juce::Rectangle<float>(16, 287, (float)getWidth() - 32, (float)getHeight() - 300);
        g.setColour(DashLookAndFeel::kBgCard);
        g.fillRoundedRectangle(quickArea, 8.0f);
        g.setColour(DashLookAndFeel::kBorderSubtle);
        g.drawRoundedRectangle(quickArea, 8.0f, 1.2f);
    }
}

void DashboardComponent::resized()
{
    // Fila superior: Selector de Modo
    easyModeBtn_.setBounds(16, 12, 220, 28);
    advancedModeBtn_.setBounds(244, 12, 240, 28);

    // Tarjeta del Motor
    engineCardTitle_.setBounds(32, 60, 300, 20);
    backendLabel_.setBounds(32, 85, 340, 18);
    deviceLabel_.setBounds(32, 105, 380, 18);
    formatLabel_.setBounds(32, 125, 340, 18);

    latencyLabel_.setBounds(430, 85, 220, 18);
    cpuLabel_.setBounds(430, 105, 220, 18);

    activateBtn_.setBounds(getWidth() - 250, 68, 220, 36);
    deactivateBtn_.setBounds(getWidth() - 250, 110, 220, 34);

    // Disposición horizontal de la Cadena Visual (9 nodos)
    const int chainStartX = 26;
    const int chainY = 195;
    const int availableW = getWidth() - 52;
    const int nodeW = (availableW - (static_cast<int>(chainNodes_.size()) - 1) * 14) / static_cast<int>(chainNodes_.size());
    const int nodeH = 54;

    for (size_t i = 0; i < chainNodes_.size(); ++i)
    {
        chainNodes_[i].bounds = juce::Rectangle<int>(chainStartX + static_cast<int>(i) * (nodeW + 14), chainY, nodeW, nodeH);
    }

    // Controles Tácticos (Easy Mode)
    const bool isEasy = (currentMode_ == DisplayMode::Easy);
    quickTitleLabel_.setVisible(isEasy);
    quickProfileSelector_.setVisible(isEasy);
    gunSlider_.setVisible(isEasy);
    gunLabel_.setVisible(isEasy);
    stepsSlider_.setVisible(isEasy);
    stepsLabel_.setVisible(isEasy);
    ambSlider_.setVisible(isEasy);
    ambLabel_.setVisible(isEasy);
    spatialToggle_.setVisible(isEasy);

    if (isEasy)
    {
        quickTitleLabel_.setBounds(32, 298, 380, 20);
        quickProfileSelector_.setBounds(getWidth() - 320, 296, 288, 28);

        const int dialY = 345;
        const int dialW = 140;
        const int dialH = 110;
        const int spacing = (getWidth() - 64 - dialW * 3) / 4;

        gunLabel_.setBounds(32 + spacing, dialY, dialW, 16);
        gunSlider_.setBounds(32 + spacing, dialY + 18, dialW, dialH);

        stepsLabel_.setBounds(32 + spacing * 2 + dialW, dialY, dialW, 16);
        stepsSlider_.setBounds(32 + spacing * 2 + dialW, dialY + 18, dialW, dialH);

        ambLabel_.setBounds(32 + spacing * 3 + dialW * 2, dialY, dialW, 16);
        ambSlider_.setBounds(32 + spacing * 3 + dialW * 2, dialY + 18, dialW, dialH);

        spatialToggle_.setBounds(getWidth() / 2 - 140, dialY + dialH + 30, 280, 26);
    }
}

void DashboardComponent::setEngineRunning(bool running)
{
    isRunning_ = running;
    activateBtn_.setColour(juce::TextButton::buttonColourId,
                           running ? DashLookAndFeel::kBgCardElevated : DashLookAndFeel::kSuccessEmerald);
    deactivateBtn_.setColour(juce::TextButton::buttonColourId,
                             running ? DashLookAndFeel::kDangerCrimson : DashLookAndFeel::kBgCardElevated);
    repaint();
}

void DashboardComponent::setDeviceName(const juce::String& name)
{
    deviceLabel_.setText("Dispositivo: " + name, juce::dontSendNotification);
}

void DashboardComponent::setFormatInfo(double sampleRate, const juce::String& layout)
{
    formatLabel_.setText("Formato: " + juce::String((int)sampleRate) + " Hz | " + layout, juce::dontSendNotification);
}

void DashboardComponent::setLatencyMs(float latencyMs)
{
    latencyLabel_.setText("Latencia: " + juce::String(latencyMs, 1) + " ms", juce::dontSendNotification);
}

void DashboardComponent::setCpuPercent(float cpuPercent)
{
    cpuLabel_.setText("Carga CPU DSP: " + juce::String(cpuPercent, 1) + " %", juce::dontSendNotification);
}

void DashboardComponent::updateQuickControls(float gunReductionDb, float footstepsDb, float ambienceReductionDb, bool spatialEnabled)
{
    gunSlider_.setValue(gunReductionDb, juce::dontSendNotification);
    stepsSlider_.setValue(footstepsDb, juce::dontSendNotification);
    ambSlider_.setValue(ambienceReductionDb, juce::dontSendNotification);
    spatialToggle_.setToggleState(spatialEnabled, juce::dontSendNotification);
}

void DashboardComponent::getQuickControlsValues(float& outGunReductionDb, float& outFootstepsDb, float& outAmbienceReductionDb, bool& outSpatialEnabled) const
{
    outGunReductionDb    = static_cast<float>(gunSlider_.getValue());
    outFootstepsDb       = static_cast<float>(stepsSlider_.getValue());
    outAmbienceReductionDb = static_cast<float>(ambSlider_.getValue());
    outSpatialEnabled    = spatialToggle_.getToggleState();
}

void DashboardComponent::setNodeBypassState(int nodeIndex, bool isBypassed)
{
    if (nodeIndex >= 0 && nodeIndex < static_cast<int>(chainNodes_.size()))
    {
        chainNodes_[static_cast<size_t>(nodeIndex)].bypassed = isBypassed;
        repaint();
    }
}

} // namespace dash::ui
