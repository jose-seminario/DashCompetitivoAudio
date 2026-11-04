#pragma once

#include <JuceHeader.h>

namespace dash::ui
{

/**
 * Tema Visual Táctico Estilo Consola de Audio (DashLookAndFeel).
 *
 * Diseñado específicamente para el Ecualizador Táctico Estéreo de 6 Bandas:
 *  - Faders verticales de consola de mezcla (carril oscuro hundido, marcas de nivel en dB,
 *    barra de neón luminosa y capuchón táctil con línea central).
 *  - Pestañas limpias de alto contraste ("ECUALIZADOR" y "AJUSTES").
 */
class DashLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DashLookAndFeel();
    ~DashLookAndFeel() override = default;

    static inline const juce::Colour kBgDark          { 0xff12141a }; // Fondo principal consola oscura
    static inline const juce::Colour kBgCard          { 0xff191c24 }; // Panel de canal / tarjeta
    static inline const juce::Colour kBgCardElevated  { 0xff222733 }; // Controles elevados
    static inline const juce::Colour kSlotRecessed    { 0xff0b0d12 }; // Carril hundido del fader
    static inline const juce::Colour kBorderSubtle    { 0xff2c3240 }; // Bordes sutiles
    static inline const juce::Colour kBorderFocus     { 0xff06b6d4 }; // Borde activo neón cian
    static inline const juce::Colour kTextMain        { 0xfff3f4f6 }; // Texto blanco nítido
    static inline const juce::Colour kTextMuted       { 0xff8b93a7 }; // Texto secundario técnico
    static inline const juce::Colour kAccentCyan      { 0xff06b6d4 }; // Neón cian primario
    static inline const juce::Colour kAccentCyanGlow  { 0xff22d3ee }; // Neón cian brillante
    static inline const juce::Colour kAccentCoral     { 0xffff384d }; // Neón coral (atenuación / bypass)
    static inline const juce::Colour kAccentCoralLight{ 0xffff5c6c }; // Coral luminoso
    static inline const juce::Colour kSuccessEmerald  { 0xff10b981 }; // Estado activo

    void drawLinearSlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPos,
        float minSliderPos,
        float maxSliderPos,
        juce::Slider::SliderStyle style,
        juce::Slider& slider) override;

    void drawToggleButton(
        juce::Graphics& g,
        juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(
        juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawComboBox(
        juce::Graphics& g,
        int width, int height,
        bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(
        juce::Graphics& g,
        const juce::Rectangle<int>& area,
        bool isSeparator,
        bool isActive,
        bool isHighlighted,
        bool isTicked,
        bool hasSubMenu,
        const juce::String& text,
        const juce::String& shortcutKeyText,
        const juce::Drawable* icon,
        const juce::Colour* textColour) override;

    void drawTabButton(
        juce::TabBarButton& button,
        juce::Graphics& g,
        bool isMouseOver,
        bool isMouseDown) override;

    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override;

    void drawTabAreaBehindFrontButton(
        juce::TabbedButtonBar& bar,
        juce::Graphics& g,
        int w,
        int h) override;

    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;
    juce::Font getTabButtonFont(juce::TabBarButton& button, float height) override;
    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace dash::ui
