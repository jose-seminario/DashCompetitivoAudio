#include "HeadphonePanel.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

HeadphonePanel::HeadphonePanel()
{
    titleLabel_.setText("CALIBRACIÓN ACÚSTICA DE AURICULARES (AUTOEQ / HARMAN 2018)", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    titleLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kAccentCyan);
    addAndMakeVisible(titleLabel_);

    modelSelectLabel_.setText("Seleccionar Modelo de Auricular:", juce::dontSendNotification);
    modelSelectLabel_.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(modelSelectLabel_);

    modelSelector_.addItem("Razer BlackShark V2 X (3.5mm Analógico)", 1);
    modelSelector_.addItem("HyperX Cloud II (Almohadillas Cuero)", 2);
    modelSelector_.addItem("Logitech G Pro X (Wired)", 3);
    modelSelector_.setSelectedId(1, juce::dontSendNotification);
    modelSelector_.onChange = [this] {
        updateModelInfo();
        if (onHeadphoneModelChanged)
            onHeadphoneModelChanged(modelSelector_.getText());
    };
    addAndMakeVisible(modelSelector_);

    statusBadgeLabel_.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    addAndMakeVisible(statusBadgeLabel_);

    manufacturerLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(manufacturerLabel_);

    targetCurveLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(targetCurveLabel_);

    measurementSourceLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(measurementSourceLabel_);

    licenseLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(licenseLabel_);

    descriptionLabel_.setFont(juce::FontOptions(11.5f));
    descriptionLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kTextMuted);
    addAndMakeVisible(descriptionLabel_);

    enableCompensationToggle_.setButtonText("Aplicar Ecualización de Compensación");
    enableCompensationToggle_.setColour(juce::ToggleButton::textColourId, DashLookAndFeel::kTextMain);
    addAndMakeVisible(enableCompensationToggle_);

    updateModelInfo();
}

void HeadphonePanel::paint(juce::Graphics& g)
{
    g.fillAll(DashLookAndFeel::kBgDark);

    const auto cardArea = juce::Rectangle<float>(16, 44, (float)getWidth() - 32, (float)getHeight() - 60);
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRoundedRectangle(cardArea, 8.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(cardArea, 8.0f, 1.2f);
}

void HeadphonePanel::resized()
{
    titleLabel_.setBounds(20, 12, 600, 24);

    const int startX = 36;
    const int contentW = getWidth() - 72;

    modelSelectLabel_.setBounds(startX, 60, contentW, 20);
    modelSelector_.setBounds(startX, 84, 460, 32);

    statusBadgeLabel_.setBounds(startX, 130, contentW, 22);
    manufacturerLabel_.setBounds(startX, 156, contentW, 20);
    targetCurveLabel_.setBounds(startX, 180, contentW, 20);
    measurementSourceLabel_.setBounds(startX, 204, contentW, 20);
    licenseLabel_.setBounds(startX, 228, contentW, 20);

    descriptionLabel_.setBounds(startX, 260, contentW, 44);

    enableCompensationToggle_.setBounds(startX, 320, 380, 26);
}

void HeadphonePanel::updateModelInfo()
{
    const int selected = modelSelector_.getSelectedId();

    if (selected == 1) // Razer BlackShark V2 X
    {
        statusBadgeLabel_.setText("ESTADO: SIN CALIBRACIÓN — PENDIENTE DE MEDICIÓN DE LABORATORIO", juce::dontSendNotification);
        statusBadgeLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kWarningAmber);

        manufacturerLabel_.setText("Fabricante: Razer Inc.", juce::dontSendNotification);
        targetCurveLabel_.setText("Target Deseado: Harman Over-Ear 2018 (Pendiente)", juce::dontSendNotification);
        measurementSourceLabel_.setText("Fuente: Ninguna medición verificable disponible públicamente en simulador estandarizado.", juce::dontSendNotification);
        licenseLabel_.setText("Licencia del Perfil: CC-BY-4.0", juce::dontSendNotification);
        descriptionLabel_.setText("Por honestidad técnica y principios de diseño, DashCompetitivoAudio no genera curvas falsas inventadas. La compensación paramétrica se mantiene inactiva hasta contar con mediciones verificadas.", juce::dontSendNotification);

        enableCompensationToggle_.setEnabled(false);
        enableCompensationToggle_.setToggleState(false, juce::dontSendNotification);
    }
    else if (selected == 2) // HyperX Cloud II
    {
        statusBadgeLabel_.setText("ESTADO: CALIBRACIÓN VERIFICADA (AUTOEQ HARMAN 2018)", juce::dontSendNotification);
        statusBadgeLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kSuccessEmerald);

        manufacturerLabel_.setText("Fabricante: HP / HyperX", juce::dontSendNotification);
        targetCurveLabel_.setText("Target: Harman Over-Ear 2018", juce::dontSendNotification);
        measurementSourceLabel_.setText("Fuente: AutoEq por Jaakko Pasanen (Archivo oratory1990)", juce::dontSendNotification);
        licenseLabel_.setText("Licencia de Filtros: MIT", juce::dontSendNotification);
        descriptionLabel_.setText("Compensación acústica de precisión con 5 filtros paramétricos para eliminar picos de resonancia en 5.8 kHz y linearizar graves.", juce::dontSendNotification);

        enableCompensationToggle_.setEnabled(true);
        enableCompensationToggle_.setToggleState(true, juce::dontSendNotification);
    }
    else if (selected == 3) // Logitech G Pro X
    {
        statusBadgeLabel_.setText("ESTADO: CALIBRACIÓN VERIFICADA (AUTOEQ HARMAN 2018)", juce::dontSendNotification);
        statusBadgeLabel_.setColour(juce::Label::textColourId, DashLookAndFeel::kSuccessEmerald);

        manufacturerLabel_.setText("Fabricante: Logitech G", juce::dontSendNotification);
        targetCurveLabel_.setText("Target: Harman Over-Ear 2018", juce::dontSendNotification);
        measurementSourceLabel_.setText("Fuente: AutoEq por Jaakko Pasanen (Archivo oratory1990)", juce::dontSendNotification);
        licenseLabel_.setText("Licencia de Filtros: MIT", juce::dontSendNotification);
        descriptionLabel_.setText("Compensación paramétrica de 4 bandas que suaviza la transición de medios-altos en 4.2 kHz y reduce fatiga auditiva.", juce::dontSendNotification);

        enableCompensationToggle_.setEnabled(true);
        enableCompensationToggle_.setToggleState(true, juce::dontSendNotification);
    }
}

} // namespace dash::ui
