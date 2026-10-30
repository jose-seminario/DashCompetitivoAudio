#include "DashLookAndFeel.h"

#include <cmath>

namespace dash::ui
{

DashLookAndFeel::DashLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, kBgDark);
    setColour(juce::ComboBox::backgroundColourId,        kBgCardElevated);
    setColour(juce::ComboBox::outlineColourId,           kBorderSubtle);
    setColour(juce::ComboBox::textColourId,              kTextMain);
    setColour(juce::ComboBox::arrowColourId,             kTextMuted);

    setColour(juce::PopupMenu::backgroundColourId,            kBgCardElevated);
    setColour(juce::PopupMenu::textColourId,                 kTextMain);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kBorderSubtle);
    setColour(juce::PopupMenu::highlightedTextColourId,       kAccentCyan);

    setColour(juce::TextButton::buttonColourId,   kBgCardElevated);
    setColour(juce::TextButton::textColourOffId,  kTextMain);
    setColour(juce::TextButton::textColourOnId,   kAccentCyan);

    setColour(juce::Label::textColourId, kTextMain);
    setColour(juce::Slider::textBoxTextColourId, kTextMain);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void DashLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 6.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Fondo circular de la perilla
    g.setColour(kBgCardElevated);
    g.fillEllipse(rx, ry, rw, rw);
    g.setColour(kBorderSubtle);
    g.drawEllipse(rx, ry, rw, rw, 1.5f);

    // 2. Arco de fondo inactivo
    const float arcRadius = radius - 4.0f;
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(kBorderSubtle.withAlpha(0.6f));
    g.strokePath(bgArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Arco de valor activo con degradado táctico cian/azul
    if (slider.isEnabled())
    {
        juce::Path activeArc;
        activeArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId, true));
        g.strokePath(activeArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. Indicador de puntero central (notch)
    juce::Path p;
    const float pointerLength = radius * 0.45f;
    const float pointerThickness = 2.5f;
    p.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 4.0f, pointerThickness, pointerLength, 1.0f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(slider.isEnabled() ? kTextMain : kTextMuted);
    g.fillPath(p);
}

void DashLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const float cornerRadius = 6.0f;

    juce::Colour baseColor = backgroundColour;
    if (shouldDrawButtonAsDown)
        baseColor = baseColor.darker(0.15f);
    else if (shouldDrawButtonAsHighlighted)
        baseColor = baseColor.brighter(0.15f);

    g.setColour(baseColor);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(shouldDrawButtonAsHighlighted ? kBorderFocus : kBorderSubtle);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.2f);
}

void DashLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool isButtonDown,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown);

    const auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height).reduced(0.5f);
    const float cornerRadius = 5.0f;

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(box.hasKeyboardFocus(true) ? kBorderFocus : box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    // Flecha indicadora estilizada hacia abajo
    const float arrowX = (float)width - 16.0f;
    const float arrowY = (float)height * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath(arrowX - 4.0f, arrowY - 2.5f);
    arrow.lineTo(arrowX, arrowY + 2.5f);
    arrow.lineTo(arrowX + 4.0f, arrowY - 2.5f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(arrow, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void DashLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    g.setColour(kBgCardElevated);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(kBorderSubtle);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

void DashLookAndFeel::drawPopupMenuItem(
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
    const juce::Colour* textColour)
{
    juce::ignoreUnused(isTicked, hasSubMenu, shortcutKeyText, icon, textColour);

    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        g.setColour(kBorderSubtle);
        g.fillRect(r.removeFromTop(1).withY(area.getCentreY()));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(kBorderSubtle);
        g.fillRoundedRectangle(area.reduced(2, 1).toFloat(), 4.0f);
    }

    auto textArea = area.reduced(10, 0);
    g.setColour(isHighlighted ? kAccentCyan : (isActive ? kTextMain : kTextMuted));
    g.setFont(juce::FontOptions(13.5f));
    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
}

juce::Font DashLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::FontOptions(13.0f);
}

} // namespace dash::ui
