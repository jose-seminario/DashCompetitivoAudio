#include "MainComponent.h"

namespace dash::ui
{

MainComponent::MainComponent()
{
    setSize(820, 620);

    // =========================================================================
    // 1. Configuración de Paleta y LookAndFeel (Estilo Oscuro Profesional)
    // =========================================================================
    auto& lf = getLookAndFeel();
    lf.setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1e1e1e));
    lf.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2d2d30));
    lf.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3f3f46));
    lf.setColour(juce::ComboBox::textColourId, juce::Colour(0xfff4f4f5));
    lf.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffa1a1aa));
    lf.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff252528));
    lf.setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff4f4f5));
    lf.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3f3f46));

    // =========================================================================
    // 2. Encabezado: Título Centrado y Subtítulo
    // =========================================================================
    titleLabel_.setText("Dash Competitivo Audio", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xfff4f4f5));
    addAndMakeVisible(titleLabel_);

    subtitleLabel_.setText("Motor Táctico de Audio Profesional • Auditoría Rara Audio", juce::dontSendNotification);
    subtitleLabel_.setFont(juce::FontOptions(13.5f));
    subtitleLabel_.setJustificationType(juce::Justification::centred);
    subtitleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));
    addAndMakeVisible(subtitleLabel_);

    // =========================================================================
    // 3. Sección de Perfiles (Auditoría Rara Audio)
    // =========================================================================
    profileSectionHeader_.setText("PERFILES ACÚSTICOS COMPETITIVOS", juce::dontSendNotification);
    profileSectionHeader_.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    profileSectionHeader_.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8)); // Sky 400
    addAndMakeVisible(profileSectionHeader_);

    profileSelectLabel_.setText("Perfil:", juce::dontSendNotification);
    profileSelectLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    profileSelectLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffe4e4e7));
    addAndMakeVisible(profileSelectLabel_);

    // Selector con las opciones auditadas: "Campaign" y "Eclipse"
    profileSelector_.addItem("Campaign", 1);
    profileSelector_.addItem("Eclipse", 2);
    profileSelector_.setSelectedId(1, juce::dontSendNotification);
    profileSelector_.onChange = [this]() {
        statusLabel_.setText("Perfil seleccionado: " + profileSelector_.getText() + " (Listo para activar)",
                             juce::dontSendNotification);
    };
    addAndMakeVisible(profileSelector_);

    // Botón Primario: Grande y llamativo ("Activar Perfil")
    activateProfileBtn_.setButtonText("Activar Perfil");
    activateProfileBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff10b981)); // Emerald 500
    activateProfileBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff059669));
    activateProfileBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    activateProfileBtn_.onClick = [this]() {
        statusLabel_.setText("Perfil [" + profileSelector_.getText() + "] marcado para activación (Fase 4: Orquestador).",
                             juce::dontSendNotification);
    };
    addAndMakeVisible(activateProfileBtn_);

    // Botón Secundario: "Desactivar / Restaurar"
    deactivateProfileBtn_.setButtonText("Desactivar / Restaurar");
    deactivateProfileBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff27272a)); // Zinc 800
    deactivateProfileBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3f3f46));
    deactivateProfileBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd4d4d8));
    deactivateProfileBtn_.onClick = [this]() {
        statusLabel_.setText("Petición de restauración recibida (Fase 4: Orquestador).",
                             juce::dontSendNotification);
    };
    addAndMakeVisible(deactivateProfileBtn_);

    // =========================================================================
    // 4. Perillas Rotatorias del Compresor (LoudnessCompressor)
    // =========================================================================
    compressorSectionHeader_.setText("PARÁMETROS DEL COMPRESOR DINÁMICO (LOUDNESS COMPRESSOR)", juce::dontSendNotification);
    compressorSectionHeader_.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    compressorSectionHeader_.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8)); // Sky 400
    addAndMakeVisible(compressorSectionHeader_);

    // Perilla 1: Intensidad (Amount) -> Default 0.75
    initRotarySlider(amountSlider_, amountLabel_, "Intensidad (Amount)",
                     0.0, 1.0, 0.001, 0.75, 2);

    // Perilla 2: Objetivo (Target) -> Default 0.277778 (Equivalente a -18 LUFS)
    initRotarySlider(targetSlider_, targetLabel_, "Objetivo (Target)",
                     0.0, 1.0, 0.000001, 0.277778, 6);

    targetLufsBadge_.setText("(-18 LUFS)", juce::dontSendNotification);
    targetLufsBadge_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    targetLufsBadge_.setJustificationType(juce::Justification::centred);
    targetLufsBadge_.setColour(juce::Label::textColourId, juce::Colour(0xff34d399)); // Emerald 400
    addAndMakeVisible(targetLufsBadge_);

    // Perilla 3: Release -> Default 0.5
    initRotarySlider(releaseSlider_, releaseLabel_, "Release",
                     0.0, 1.0, 0.001, 0.5, 2);

    // =========================================================================
    // 5. Barra de Estado Inferior
    // =========================================================================
    statusLabel_.setText("● Sistema listo • Modo Visual Fase 2 activo", juce::dontSendNotification);
    statusLabel_.setFont(juce::FontOptions(12.5f));
    statusLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff18181b));
    statusLabel_.setColour(juce::Label::outlineColourId, juce::Colour(0xff27272a));
    statusLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));
    addAndMakeVisible(statusLabel_);
}

void MainComponent::initRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& text,
                                     double minVal, double maxVal, double step, double defaultVal,
                                     int decimalPlaces)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 22);
    slider.setRange(minVal, maxVal, step);
    slider.setValue(defaultVal, juce::dontSendNotification);
    slider.setNumDecimalPlacesToDisplay(decimalPlaces);

    // Estilo visual del Dial
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff0284c7)); // Sky 600
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff27272a)); // Zinc 800
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff38bdf8)); // Sky 400
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff18181b));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3f3f46));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff4f4f5));
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffe4e4e7));
    addAndMakeVisible(label);
}

void MainComponent::paint(juce::Graphics& g)
{
    // 1. Fondo Oscuro Principal (#1E1E1E)
    g.fillAll(juce::Colour(0xff1e1e1e));

    // 2. Paneles con fondo oscuro elevado (#252528) y bordes redondeados (#38383c)
    g.setColour(juce::Colour(0xff252528));
    g.fillRoundedRectangle(profilePanelBounds_.toFloat(), 8.0f);
    g.fillRoundedRectangle(compressorPanelBounds_.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff38383c));
    g.drawRoundedRectangle(profilePanelBounds_.toFloat(), 8.0f, 1.2f);
    g.drawRoundedRectangle(compressorPanelBounds_.toFloat(), 8.0f, 1.2f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(24);

    // Encabezado Centrado
    auto headerArea = area.removeFromTop(62);
    titleLabel_.setBounds(headerArea.removeFromTop(34));
    subtitleLabel_.setBounds(headerArea);

    area.removeFromTop(12);

    // Barra de Estado Inferior
    statusLabel_.setBounds(area.removeFromBottom(30));
    area.removeFromBottom(14);

    // Panel 1: Controles de Perfil
    profilePanelBounds_ = area.removeFromTop(135);
    auto pInner = profilePanelBounds_.reduced(18, 14);
    profileSectionHeader_.setBounds(pInner.removeFromTop(20));
    pInner.removeFromTop(12);

    auto pControlsRow = pInner.removeFromTop(44);
    profileSelectLabel_.setBounds(pControlsRow.removeFromLeft(48));
    profileSelector_.setBounds(pControlsRow.removeFromLeft(190).reduced(0, 4));
    pControlsRow.removeFromLeft(20);

    // Botones con dimensiones destacadas
    activateProfileBtn_.setBounds(pControlsRow.removeFromLeft(220));
    pControlsRow.removeFromLeft(12);
    deactivateProfileBtn_.setBounds(pControlsRow.removeFromLeft(200));

    area.removeFromTop(16);

    // Panel 2: Perillas del Compresor
    compressorPanelBounds_ = area;
    auto cInner = compressorPanelBounds_.reduced(18, 14);
    compressorSectionHeader_.setBounds(cInner.removeFromTop(20));
    cInner.removeFromTop(10);

    // 3 Columnas simétricas para las perillas
    const int colWidth = cInner.getWidth() / 3;

    // Columna 1: Amount
    auto col1 = cInner.removeFromLeft(colWidth).reduced(10, 0);
    amountLabel_.setBounds(col1.removeFromTop(24));
    col1.removeFromTop(4);
    amountSlider_.setBounds(col1.removeFromTop(150));

    // Columna 2: Target
    auto col2 = cInner.removeFromLeft(colWidth).reduced(10, 0);
    auto targetHeaderRow = col2.removeFromTop(24);
    targetLabel_.setBounds(targetHeaderRow.removeFromTop(14));
    targetLufsBadge_.setBounds(targetHeaderRow);
    col2.removeFromTop(4);
    targetSlider_.setBounds(col2.removeFromTop(150));

    // Columna 3: Release
    auto col3 = cInner.reduced(10, 0);
    releaseLabel_.setBounds(col3.removeFromTop(24));
    col3.removeFromTop(4);
    releaseSlider_.setBounds(col3.removeFromTop(150));
}

} // namespace dash::ui
