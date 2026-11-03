#include "DashLookAndFeel.h"

#include <cmath>

namespace dash::ui
{

DashLookAndFeel::DashLookAndFeel()
{
    // Tipografía minimalista y moderna predeterminada
    setDefaultSansSerifTypefaceName("Segoe UI");

    setColour(juce::ResizableWindow::backgroundColourId, kBgDark);
    setColour(juce::ComboBox::backgroundColourId,        kBgCardElevated);
    setColour(juce::ComboBox::outlineColourId,           kBorderSubtle);
    setColour(juce::ComboBox::textColourId,              kTextMain);
    setColour(juce::ComboBox::arrowColourId,             kAccentCoralLight);

    setColour(juce::PopupMenu::backgroundColourId,            kBgCardElevated);
    setColour(juce::PopupMenu::textColourId,                 kTextMain);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kBorderSubtle);
    setColour(juce::PopupMenu::highlightedTextColourId,       kAccentCoralLight);

    setColour(juce::TextButton::buttonColourId,   kBgCardElevated);
    setColour(juce::TextButton::textColourOffId,  kTextMain);
    setColour(juce::TextButton::textColourOnId,   kAccentCoralLight);

    setColour(juce::Label::textColourId, kTextMain);
    setColour(juce::Slider::textBoxTextColourId, kTextMain);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::rotarySliderFillColourId, kAccentCoral);
    setColour(juce::Slider::trackColourId, kPillTrackDark);
}

void DashLookAndFeel::drawLinearSlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float minSliderPos,
    float maxSliderPos,
    juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    if (width <= 0 || height <= 0)
        return;
    juce::ignoreUnused(minSliderPos, maxSliderPos, style);

    const bool isVertical = slider.isVertical();
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);

    if (isVertical)
    {
        // Fader Vertical en Cápsula (Estilo idéntico a los 5 faders de la referencia)
        const float trackWidth = juce::jlimit(20.0f, 34.0f, (float)width * 0.44f);
        const float centreX = bounds.getCentreX();
        const float trackRadius = trackWidth * 0.5f;

        const auto trackBounds = juce::Rectangle<float>(
            centreX - trackRadius,
            bounds.getY() + 8.0f,
            trackWidth,
            bounds.getHeight() - 16.0f);

        // 1. Pista cápsula inactiva (Pastilla oscura suave con borde)
        g.setColour(slider.findColour(juce::Slider::trackColourId, true));
        g.fillRoundedRectangle(trackBounds, trackRadius);
        g.setColour(kBorderSubtle);
        g.drawRoundedRectangle(trackBounds, trackRadius, 1.2f);

        // 2. Relleno activo en cápsula con degradado Coral Neón (#FF5C6C -> #FF384D)
        const float clampedPos = juce::jlimit(trackBounds.getY(), trackBounds.getBottom(), sliderPos);
        const float fillHeight = trackBounds.getBottom() - clampedPos;

        if (fillHeight > 1.0f)
        {
            const auto fillBounds = juce::Rectangle<float>(
                trackBounds.getX(),
                clampedPos,
                trackWidth,
                fillHeight);

            juce::Path trackPath;
            trackPath.addRoundedRectangle(trackBounds, trackRadius);

            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(trackPath);

            juce::ColourGradient grad(
                kAccentCoralLight, centreX, clampedPos,
                kAccentCoral, centreX, trackBounds.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(fillBounds, trackRadius);
        }

        // 3. Perilla circular (Thumb) con tacto 3D y aro de brillo
        const float thumbDiameter = trackWidth * 1.06f;
        const float thumbRadius = thumbDiameter * 0.5f;
        const float thumbY = clampedPos;

        // Sombra suave de la perilla
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillEllipse(centreX - thumbRadius, thumbY - thumbRadius + 2.5f, thumbDiameter, thumbDiameter);

        // Cuerpo de la perilla circular en coral luminoso
        juce::ColourGradient thumbGrad(
            kAccentCoralLight.brighter(0.14f), centreX, thumbY - thumbRadius,
            kAccentCoral, centreX, thumbY + thumbRadius, false);
        g.setGradientFill(thumbGrad);
        g.fillEllipse(centreX - thumbRadius, thumbY - thumbRadius, thumbDiameter, thumbDiameter);

        // Anillo de brillo blanco translúcido
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.drawEllipse(centreX - thumbRadius, thumbY - thumbRadius, thumbDiameter, thumbDiameter, 1.4f);
    }
    else
    {
        // Slider Horizontal encapsulado
        const float trackHeight = juce::jlimit(12.0f, 22.0f, (float)height * 0.36f);
        const float centreY = bounds.getCentreY();
        const float trackRadius = trackHeight * 0.5f;

        const auto trackBounds = juce::Rectangle<float>(
            bounds.getX() + 6.0f,
            centreY - trackRadius,
            bounds.getWidth() - 12.0f,
            trackHeight);

        // 1. Pista inactiva
        g.setColour(slider.findColour(juce::Slider::trackColourId, true));
        g.fillRoundedRectangle(trackBounds, trackRadius);
        g.setColour(kBorderSubtle);
        g.drawRoundedRectangle(trackBounds, trackRadius, 1.2f);

        // 2. Relleno activo con degradado coral horizontal
        const float clampedPos = juce::jlimit(trackBounds.getX(), trackBounds.getRight(), sliderPos);
        const float fillWidth = clampedPos - trackBounds.getX();

        if (fillWidth > 1.0f)
        {
            const auto fillBounds = juce::Rectangle<float>(
                trackBounds.getX(),
                trackBounds.getY(),
                fillWidth,
                trackHeight);

            juce::Path trackPath;
            trackPath.addRoundedRectangle(trackBounds, trackRadius);

            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(trackPath);

            juce::ColourGradient grad(
                kAccentCoral, trackBounds.getX(), centreY,
                kAccentCoralLight, clampedPos, centreY, false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(fillBounds, trackRadius);
        }

        // 3. Perilla circular
        const float thumbDiameter = trackHeight * 1.45f;
        const float thumbRadius = thumbDiameter * 0.5f;
        const float thumbX = clampedPos;

        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillEllipse(thumbX - thumbRadius, centreY - thumbRadius + 2.0f, thumbDiameter, thumbDiameter);

        juce::ColourGradient thumbGrad(
            kAccentCoralLight.brighter(0.14f), thumbX, centreY - thumbRadius,
            kAccentCoral, thumbX, centreY + thumbRadius, false);
        g.setGradientFill(thumbGrad);
        g.fillEllipse(thumbX - thumbRadius, centreY - thumbRadius, thumbDiameter, thumbDiameter);

        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawEllipse(thumbX - thumbRadius, centreY - thumbRadius, thumbDiameter, thumbDiameter, 1.4f);
    }
}

void DashLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    if (width <= 0 || height <= 0)
        return;
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
    g.drawEllipse(rx, ry, rw, rw, 1.4f);

    // 2. Arco de fondo inactivo
    const float arcRadius = radius - 4.5f;
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(kBorderSubtle.withAlpha(0.7f));
    g.strokePath(bgArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Arco de valor activo con degradado táctico Coral Neón
    if (slider.isEnabled())
    {
        juce::Path activeArc;
        activeArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(kAccentCoralLight);
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

void DashLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    const auto bounds = button.getLocalBounds().toFloat();
    const bool isToggled = button.getToggleState();

    // Switch tipo cápsula moderno (inspirado en la referencia)
    const float switchW = 44.0f;
    const float switchH = 24.0f;
    const float switchY = bounds.getCentreY() - switchH * 0.5f;
    const auto switchArea = juce::Rectangle<float>(bounds.getX() + 2.0f, switchY, switchW, switchH);
    const float cornerRadius = switchH * 0.5f;

    if (isToggled)
    {
        // Fondo activo: Degradado Coral Neón (#FF5C6C -> #FF384D)
        juce::ColourGradient grad(
            kAccentCoralLight, switchArea.getX(), switchArea.getY(),
            kAccentCoral, switchArea.getRight(), switchArea.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(switchArea, cornerRadius);
        g.setColour(kAccentCoralLight.withAlpha(0.4f));
        g.drawRoundedRectangle(switchArea, cornerRadius, 1.2f);
    }
    else
    {
        // Fondo inactivo: Cápsula oscura elevada
        g.setColour(kBgCardElevated);
        g.fillRoundedRectangle(switchArea, cornerRadius);
        g.setColour(kBorderSubtle);
        g.drawRoundedRectangle(switchArea, cornerRadius, 1.2f);
    }

    // Thumb circular deslizante blanco puro
    const float thumbSize = switchH - 6.0f;
    const float thumbX = isToggled ? (switchArea.getRight() - thumbSize - 3.0f) : (switchArea.getX() + 3.0f);
    const float thumbY = switchArea.getY() + 3.0f;

    // Sombra suave del thumb
    g.setColour(juce::Colours::black.withAlpha(0.30f));
    g.fillEllipse(thumbX, thumbY + 1.2f, thumbSize, thumbSize);

    // Cuerpo del thumb en blanco
    g.setColour(kPillWhite);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    // Texto de la etiqueta al lado derecho
    if (button.getButtonText().isNotEmpty())
    {
        const auto textArea = bounds.withTrimmedLeft(switchW + 12.0f);
        g.setColour(button.isEnabled() ? kTextMain : kTextMuted);
        g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f).withStyle(isToggled ? "Bold" : "Plain")));
        g.drawFittedText(button.getButtonText(), textArea.toNearestInt(), juce::Justification::centredLeft, 1);
    }
}

void DashLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool isCircular = (std::abs(bounds.getWidth() - bounds.getHeight()) <= 6.0f);
    const float cornerRadius = isCircular ? (bounds.getHeight() * 0.5f) : (bounds.getHeight() * 0.5f); // Full capsule pill

    juce::Colour baseColor = backgroundColour;

    if (shouldDrawButtonAsDown)
    {
        g.setColour(baseColor.darker(0.12f));
        if (isCircular)
            g.fillEllipse(bounds);
        else
            g.fillRoundedRectangle(bounds, cornerRadius);

        // Brillo interior coral reactivo a la pulsación
        g.setColour(kAccentCoral.withAlpha(0.25f));
        if (isCircular)
            g.fillEllipse(bounds.reduced(1.2f));
        else
            g.fillRoundedRectangle(bounds.reduced(1.2f), cornerRadius - 1.0f);

        g.setColour(kAccentCoral);
        if (isCircular)
            g.drawEllipse(bounds, 1.4f);
        else
            g.drawRoundedRectangle(bounds, cornerRadius, 1.4f);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(baseColor.brighter(0.08f));
        if (isCircular)
            g.fillEllipse(bounds);
        else
            g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(kAccentCoralLight);
        if (isCircular)
            g.drawEllipse(bounds, 1.2f);
        else
            g.drawRoundedRectangle(bounds, cornerRadius, 1.2f);
    }
    else
    {
        g.setColour(baseColor);
        if (isCircular)
            g.fillEllipse(bounds);
        else
            g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(kBorderSubtle);
        if (isCircular)
            g.drawEllipse(bounds, 1.0f);
        else
            g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    }
}

void DashLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool isButtonDown,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box)
{
    if (width <= 0 || height <= 0)
        return;
    juce::ignoreUnused(isButtonDown);

    const auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height).reduced(0.5f);
    const float cornerRadius = bounds.getHeight() * 0.5f; // Forma de píldora redondeada completa

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(box.hasKeyboardFocus(true) ? kBorderFocus : box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    // Flecha indicadora estilizada hacia abajo en Coral
    const float arrowX = (float)width - 18.0f;
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
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(kBorderSubtle);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);
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
        g.fillRoundedRectangle(area.reduced(4, 1).toFloat(), 6.0f);
    }

    auto textArea = area.reduced(12, 0);
    g.setColour(isHighlighted ? kAccentCoralLight : (isActive ? kTextMain : kTextMuted));
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.5f)));
    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
}

juce::Font DashLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(juce::jmin(13.5f, (float)buttonHeight * 0.50f)).withStyle("Bold"));
}

juce::Font DashLookAndFeel::getTabButtonFont(juce::TabBarButton&, float height)
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(juce::jmin(12.5f, height * 0.44f)).withStyle("Bold"));
}

juce::Font DashLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f));
}

juce::Font DashLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f));
}

juce::Font DashLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f));
}

} // namespace dash::ui
