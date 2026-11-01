#pragma once

#include <JuceHeader.h>

namespace dash::ui
{

/**
 * Tema Visual Táctico Profesional (DashLookAndFeel).
 *
 * Proporciona una paleta oscura moderna (#121214, #1A1A1E), controles rotativos
 * vectorizados de precisión, botones planos tácticos y menús con estilo visual uniforme.
 * Tipografía minimalista y moderna basada en Segoe UI.
 */
class DashLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DashLookAndFeel();
    ~DashLookAndFeel() override = default;

    // Colores semánticos del sistema
    static inline const juce::Colour kBgDark          { 0xff121214 }; // Fondo de ventana
    static inline const juce::Colour kBgCard          { 0xff1a1a1e }; // Fondo de tarjetas
    static inline const juce::Colour kBgCardElevated  { 0xff242429 }; // Fondo elevado
    static inline const juce::Colour kBorderSubtle    { 0xff2a2a30 }; // Bordes sutiles
    static inline const juce::Colour kBorderFocus     { 0xff0284c7 }; // Borde activo
    static inline const juce::Colour kTextMain        { 0xfff4f4f5 }; // Texto blanco primario
    static inline const juce::Colour kTextMuted       { 0xffa1a1aa }; // Texto gris secundario
    static inline const juce::Colour kAccentCyan      { 0xff06b6d4 }; // Acento cian táctico
    static inline const juce::Colour kAccentBlue      { 0xff0284c7 }; // Acento azul primario
    static inline const juce::Colour kSuccessEmerald  { 0xff10b981 }; // Éxito / Activo
    static inline const juce::Colour kWarningAmber    { 0xfff59e0b }; // Alerta / Advertencia
    static inline const juce::Colour kDangerCrimson   { 0xffef4444 }; // Peligro / Bypass

    void drawRotarySlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider& slider) override;

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

    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;
    juce::Font getTabButtonFont(juce::TabBarButton& button, float height) override;
    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace dash::ui
