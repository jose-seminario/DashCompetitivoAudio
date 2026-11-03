#pragma once

#include <JuceHeader.h>

namespace dash::ui
{

/**
 * Tema Visual Táctico Profesional (DashLookAndFeel).
 *
 * Basado en la estética Soft Dark / Neon Coral Capsule:
 *  - Paleta Slate carbón mate (#1E2026, #16181D)
 *  - Degradado Coral Neón / Sunset Red (#FF5C6C -> #FF384D)
 *  - Faders verticales y horizontales en cápsula con perilla 3D
 *  - Switches toggle tipo pastilla/cápsula moderna
 *  - Botones y campos de búsqueda redondeados (pill shape)
 *  - Tipografía minimalista basada en Segoe UI
 */
class DashLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DashLookAndFeel();
    ~DashLookAndFeel() override = default;

    // Colores semánticos del sistema (Paleta Soft Dark / Neon Coral de referencia)
    static inline const juce::Colour kBgDark          { 0xff1e2026 }; // Fondo de ventana carbón mate
    static inline const juce::Colour kBgCard          { 0xff16181d }; // Fondo de tarjetas oscuras
    static inline const juce::Colour kBgCardElevated  { 0xff252831 }; // Fondo elevado / faders / inputs
    static inline const juce::Colour kBorderSubtle    { 0xff2e323c }; // Bordes sutiles
    static inline const juce::Colour kBorderFocus     { 0xffff4d61 }; // Borde activo en coral
    static inline const juce::Colour kTextMain        { 0xffffffff }; // Texto blanco primario
    static inline const juce::Colour kTextMuted       { 0xff8e95a5 }; // Texto gris secundario
    static inline const juce::Colour kAccentCoral     { 0xffff384d }; // Rojo coral / Sunset primario
    static inline const juce::Colour kAccentCoralLight{ 0xffff5c6c }; // Coral brillante superior
    static inline const juce::Colour kAccentCyan      { 0xff06b6d4 }; // Acento cian táctico secundario
    static inline const juce::Colour kAccentBlue      { 0xff0284c7 }; // Acento azul
    static inline const juce::Colour kSuccessEmerald  { 0xff10b981 }; // Éxito / Activo
    static inline const juce::Colour kWarningAmber    { 0xfff59e0b }; // Alerta / Advertencia
    static inline const juce::Colour kDangerCrimson   { 0xffef4444 }; // Peligro / Bypass
    static inline const juce::Colour kPillWhite       { 0xffffffff }; // Blanco cápsula
    static inline const juce::Colour kPillTrackLight  { 0xffe2e5ec }; // Pista clara para faders
    static inline const juce::Colour kPillTrackDark   { 0xff272a33 }; // Pista oscura para faders

    void drawLinearSlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPos,
        float minSliderPos,
        float maxSliderPos,
        juce::Slider::SliderStyle style,
        juce::Slider& slider) override;

    void drawRotarySlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
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

    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;
    juce::Font getTabButtonFont(juce::TabBarButton& button, float height) override;
    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace dash::ui
