#pragma once

#include <JuceHeader.h>

namespace dash::ui
{

/**
 * Tema Visual Táctico Cyberpunk / Hacker Gamer / Sci-Fi HUD (DashLookAndFeel).
 *
 * Implementa de forma nativa en el motor vectorial de JUCE:
 *  - Geometría con esquinas biseladas a 45° (clip-path / chamfered polygon).
 *  - Brackets angulares Sci-Fi en esquinas y líneas de escaneo (scanlines).
 *  - Paleta Híbrida Gamer Táctica: Negro obsidiana, rojo carmesí neón, rojo rubí metálico,
 *    verde esmeralda neón y cian neón para telemetría y datos.
 *  - Tipografía técnica monoespaciada en mayúsculas compactas (Consolas / Fira Code).
 */
class DashLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DashLookAndFeel();
    ~DashLookAndFeel() override = default;

    // Paleta Cyberpunk / Sci-Fi HUD
    static inline const juce::Colour kBgObsidian       { 0xff070204 }; // Negro obsidiana base
    static inline const juce::Colour kBgDark           { 0xff0c0407 }; // Fondo de ventana
    static inline const juce::Colour kBgCard           { 0xff160509 }; // Panel oscuro con tinte carmesí
    static inline const juce::Colour kBgCardElevated   { 0xff250810 }; // Tarjeta / botón elevado
    static inline const juce::Colour kSlotRecessed     { 0xff040102 }; // Ranura fader hundida

    static inline const juce::Colour kNeonCrimson      { 0xffff1a40 }; // Rojo carmesí neón principal
    static inline const juce::Colour kNeonCrimsonGlow  { 0xffff3d5e }; // Resplandor carmesí
    static inline const juce::Colour kMetallicRuby     { 0xff8b0000 }; // Rojo rubí metálico oscuro
    static inline const juce::Colour kRubyBorder       { 0xff5a0a16 }; // Borde estructural

    static inline const juce::Colour kNeonCyan         { 0xff00e5ff }; // Cian neón (frecuencias / chips)
    static inline const juce::Colour kNeonCyanGlow     { 0xff5cf0ff }; // Cian neón brillante
    static inline const juce::Colour kNeonEmerald      { 0xff00ff88 }; // Verde esmeralda neón (óptimo / activo)
    static inline const juce::Colour kNeonAmber        { 0xffffb700 }; // Ámbar de advertencia / backup

    static inline const juce::Colour kTextMain         { 0xfff8f9fa }; // Texto blanco nítido
    static inline const juce::Colour kTextMuted        { 0xffa88b94 }; // Texto técnico atenuado

    // Utilidades de dibujo HUD Sci-Fi
    static juce::Path createChamferedPath(
        juce::Rectangle<float> bounds,
        float cutSize,
        bool cutTopLeft = true,
        bool cutTopRight = true,
        bool cutBottomRight = true,
        bool cutBottomLeft = true);

    static void drawHudPanel(
        juce::Graphics& g,
        juce::Rectangle<float> bounds,
        float cutSize = 12.0f,
        bool drawBrackets = true,
        bool drawScanlines = true,
        juce::Colour bgColour = kBgCard,
        juce::Colour borderColour = kNeonCrimson,
        float strokeWidth = 1.2f);

    static void drawCornerBrackets(
        juce::Graphics& g,
        juce::Rectangle<float> bounds,
        float bracketSize = 10.0f,
        float cutOffset = 4.0f,
        juce::Colour bracketColour = kNeonCrimsonGlow,
        float strokeWidth = 1.8f);

    static void drawScanlines(
        juce::Graphics& g,
        juce::Rectangle<float> bounds,
        float spacing = 3.5f,
        float alpha = 0.045f);

    // Componentes nativos JUCE sobreescritos con estética Cyberpunk HUD
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

    void drawButtonText(
        juce::Graphics& g,
        juce::TextButton& button,
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
    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;

private:
    static juce::Font getMonospaceFont(float height, bool isBold = false);
};

} // namespace dash::ui
