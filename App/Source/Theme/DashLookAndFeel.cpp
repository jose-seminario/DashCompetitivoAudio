#include "DashLookAndFeel.h"

#include <cmath>

namespace dash::ui
{

DashLookAndFeel::DashLookAndFeel()
{
    setDefaultSansSerifTypefaceName("Consolas");

    setColour(juce::ResizableWindow::backgroundColourId, kBgObsidian);
    setColour(juce::TabbedComponent::backgroundColourId, kBgObsidian);
    setColour(juce::TabbedComponent::outlineColourId,    juce::Colours::transparentBlack);

    setColour(juce::ComboBox::backgroundColourId,        kBgCardElevated);
    setColour(juce::ComboBox::outlineColourId,           kRubyBorder);
    setColour(juce::ComboBox::textColourId,              kTextMain);
    setColour(juce::ComboBox::arrowColourId,             kNeonCrimsonGlow);

    setColour(juce::PopupMenu::backgroundColourId,            kBgCardElevated);
    setColour(juce::PopupMenu::textColourId,                 kTextMain);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kRubyBorder);
    setColour(juce::PopupMenu::highlightedTextColourId,       kNeonCrimsonGlow);

    setColour(juce::TextButton::buttonColourId,   kBgCardElevated);
    setColour(juce::TextButton::textColourOffId,  kTextMain);
    setColour(juce::TextButton::textColourOnId,   kNeonCrimsonGlow);

    setColour(juce::Label::textColourId, kTextMain);
    setColour(juce::Slider::textBoxTextColourId,       kNeonCyan);
    setColour(juce::Slider::textBoxBackgroundColourId, kSlotRecessed);
    setColour(juce::Slider::textBoxOutlineColourId,    kRubyBorder);
    setColour(juce::Slider::trackColourId,             kNeonCrimson);
}

juce::Font DashLookAndFeel::getMonospaceFont(float height, bool isBold)
{
    return juce::Font(juce::FontOptions()
        .withName("Consolas")
        .withHeight(height)
        .withStyle(isBold ? "Bold" : "Plain"));
}

juce::Path DashLookAndFeel::createChamferedPath(
    juce::Rectangle<float> bounds,
    float cutSize,
    bool cutTopLeft,
    bool cutTopRight,
    bool cutBottomRight,
    bool cutBottomLeft)
{
    juce::Path p;
    const float l = bounds.getX();
    const float t = bounds.getY();
    const float r = bounds.getRight();
    const float b = bounds.getBottom();

    const float ctl = cutTopLeft     ? juce::jmin(cutSize, bounds.getWidth() * 0.45f, bounds.getHeight() * 0.45f) : 0.0f;
    const float ctr = cutTopRight    ? juce::jmin(cutSize, bounds.getWidth() * 0.45f, bounds.getHeight() * 0.45f) : 0.0f;
    const float cbr = cutBottomRight ? juce::jmin(cutSize, bounds.getWidth() * 0.45f, bounds.getHeight() * 0.45f) : 0.0f;
    const float cbl = cutBottomLeft  ? juce::jmin(cutSize, bounds.getWidth() * 0.45f, bounds.getHeight() * 0.45f) : 0.0f;

    p.startNewSubPath(l + ctl, t);
    p.lineTo(r - ctr, t);
    if (ctr > 0.0f) p.lineTo(r, t + ctr);
    p.lineTo(r, b - cbr);
    if (cbr > 0.0f) p.lineTo(r - cbr, b);
    p.lineTo(l + cbl, b);
    if (cbl > 0.0f) p.lineTo(l, b - cbl);
    p.lineTo(l, t + ctl);
    p.closeSubPath();

    return p;
}

void DashLookAndFeel::drawCornerBrackets(
    juce::Graphics& g,
    juce::Rectangle<float> bounds,
    float bracketSize,
    float cutOffset,
    juce::Colour bracketColour,
    float strokeWidth)
{
    const float l = bounds.getX();
    const float t = bounds.getY();
    const float r = bounds.getRight();
    const float b = bounds.getBottom();

    g.setColour(bracketColour);

    // Top-Left
    g.drawLine(l, t + cutOffset + bracketSize, l, t + cutOffset, strokeWidth);
    g.drawLine(l + cutOffset, t, l + cutOffset + bracketSize, t, strokeWidth);

    // Top-Right
    g.drawLine(r, t + cutOffset + bracketSize, r, t + cutOffset, strokeWidth);
    g.drawLine(r - cutOffset - bracketSize, t, r - cutOffset, t, strokeWidth);

    // Bottom-Right
    g.drawLine(r, b - cutOffset - bracketSize, r, b - cutOffset, strokeWidth);
    g.drawLine(r - cutOffset - bracketSize, b, r - cutOffset, b, strokeWidth);

    // Bottom-Left
    g.drawLine(l, b - cutOffset - bracketSize, l, b - cutOffset, strokeWidth);
    g.drawLine(l + cutOffset, b, l + cutOffset + bracketSize, b, strokeWidth);
}

void DashLookAndFeel::drawScanlines(
    juce::Graphics& g,
    juce::Rectangle<float> bounds,
    float spacing,
    float alpha)
{
    g.setColour(juce::Colours::white.withAlpha(alpha));
    const float bottom = bounds.getBottom();
    for (float y = bounds.getY(); y < bottom; y += spacing)
    {
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
    }
}

void DashLookAndFeel::drawHudPanel(
    juce::Graphics& g,
    juce::Rectangle<float> bounds,
    float cutSize,
    bool drawBrackets,
    bool drawScanlinesFlag,
    juce::Colour bgColour,
    juce::Colour borderColour,
    float strokeWidth)
{
    if (bounds.getWidth() <= 4.0f || bounds.getHeight() <= 4.0f)
        return;

    auto panelPath = createChamferedPath(bounds, cutSize);

    // 1. Fondo oscuro obsidiana / carmesí
    g.setColour(bgColour);
    g.fillPath(panelPath);

    // 2. Textura sutil de líneas de escaneo (Scanlines)
    if (drawScanlinesFlag)
    {
        juce::Graphics::ScopedSaveState scanState(g);
        g.reduceClipRegion(panelPath);
        drawScanlines(g, bounds, 3.5f, 0.04f);
    }

    // 3. Resplandor exterior y borde biselado
    g.setColour(borderColour.withAlpha(0.22f));
    g.strokePath(panelPath, juce::PathStrokeType(strokeWidth + 2.5f));

    g.setColour(borderColour);
    g.strokePath(panelPath, juce::PathStrokeType(strokeWidth));

    // 4. Brackets angulares en las 4 esquinas
    if (drawBrackets)
    {
        drawCornerBrackets(g, bounds, cutSize * 0.9f, cutSize * 0.4f, borderColour, strokeWidth * 1.3f);
    }
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

    const auto bounds = juce::Rectangle<float>(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height));

    if (slider.isVertical())
    {
        const float padTop    = 14.0f;
        const float padBottom = 14.0f;
        const float slotWidth = juce::jlimit(8.0f, 14.0f, bounds.getWidth() * 0.16f);
        const float centreX   = bounds.getCentreX();
        const float slotTop   = bounds.getY() + padTop;
        const float slotH     = juce::jmax(24.0f, bounds.getHeight() - padTop - padBottom);
        const float slotBot   = slotTop + slotH;

        const juce::Rectangle<float> slotBounds(
            centreX - slotWidth * 0.5f,
            slotTop,
            slotWidth,
            slotH);

        // 1. Escala de Ticks Tácticos Cyberpunk (-18 a +18 dB)
        constexpr int kNumTicks = 13;
        const float tickInnerGap = slotWidth * 0.5f + 5.0f;

        for (int i = 0; i < kNumTicks; ++i)
        {
            const float norm = static_cast<float>(i) / static_cast<float>(kNumTicks - 1);
            const float ty   = slotTop + norm * slotH;
            const bool isZeroCenter = (i == (kNumTicks / 2));
            const bool isMajor      = ((i % 2) == 0);

            const float tickLen = isZeroCenter ? 12.0f : (isMajor ? 7.0f : 4.0f);
            const float strokeW = isZeroCenter ? 1.8f : 1.0f;

            if (isZeroCenter)
                g.setColour(kNeonCyanGlow);
            else if (isMajor)
                g.setColour(kNeonCrimson.withAlpha(0.65f));
            else
                g.setColour(kRubyBorder.withAlpha(0.55f));

            g.drawLine(centreX - tickInnerGap - tickLen, ty, centreX - tickInnerGap, ty, strokeW);
            g.drawLine(centreX + tickInnerGap, ty, centreX + tickInnerGap + tickLen, ty, strokeW);
        }

        // 2. Ranura hundida del fader (ranura metálica obsidiana biselada)
        auto slotPath = createChamferedPath(slotBounds, 3.0f);
        g.setColour(kSlotRecessed);
        g.fillPath(slotPath);
        g.setColour(kRubyBorder);
        g.strokePath(slotPath, juce::PathStrokeType(1.0f));

        // 3. Barra luminosa reactiva (Neón Carmesí / Neón Verde)
        const float clampedPos = juce::jlimit(slotTop, slotBot, sliderPos);
        const float zeroY      = slotTop + slotH * 0.5f;

        if (slider.isEnabled())
        {
            juce::Graphics::ScopedSaveState clipState(g);
            g.reduceClipRegion(slotPath);

            const float topY = juce::jmin(zeroY, clampedPos);
            const float botY = juce::jmax(zeroY, clampedPos);
            const float barH = botY - topY;

            // Relleno tenue de fondo
            g.setColour(kNeonCrimson.withAlpha(0.12f));
            g.fillRect(slotBounds.getX(), clampedPos, slotWidth, slotBot - clampedPos);

            if (barH > 0.8f)
            {
                const bool isBoost = (clampedPos < zeroY);
                const juce::Colour cTop = isBoost ? kNeonEmerald : kNeonCrimsonGlow;
                const juce::Colour cBot = isBoost ? kNeonCyan    : kMetallicRuby;

                juce::ColourGradient neonGrad(
                    cTop, centreX, topY,
                    cBot, centreX, botY,
                    false);
                g.setGradientFill(neonGrad);
                g.fillRect(slotBounds.getX(), topY, slotWidth, barH);
            }
        }

        // 4. Capuchón angular Sci-Fi con esquinas biseladas a 45°
        const float capW = juce::jlimit(28.0f, 44.0f, bounds.getWidth() * 0.58f);
        const float capH = 18.0f;
        const juce::Rectangle<float> capBounds(
            centreX - capW * 0.5f,
            clampedPos - capH * 0.5f,
            capW,
            capH);

        auto capPath = createChamferedPath(capBounds, 4.0f);

        // Sombra / resplandor
        g.setColour(juce::Colours::black.withAlpha(0.70f));
        g.fillPath(createChamferedPath(capBounds.translated(0.0f, 2.0f), 4.0f));

        // Cuerpo metálico biselado
        juce::ColourGradient capGrad(
            juce::Colour(0xff2d0b13), centreX, capBounds.getY(),
            juce::Colour(0xff120306), centreX, capBounds.getBottom(),
            false);
        g.setGradientFill(capGrad);
        g.fillPath(capPath);

        // Borde neón carmesí con resplandor
        g.setColour(slider.isEnabled() ? kNeonCrimsonGlow : kRubyBorder);
        g.strokePath(capPath, juce::PathStrokeType(1.2f));

        // Línea central luminosa indicadora (verde esmeralda o cian neón)
        const juce::Colour indicatorCol = slider.isEnabled()
            ? (std::abs(clampedPos - zeroY) < 1.0f ? kNeonCyanGlow : kNeonEmerald)
            : kTextMuted;

        g.setColour(indicatorCol);
        g.drawLine(
            capBounds.getX() + 5.0f,
            clampedPos,
            capBounds.getRight() - 5.0f,
            clampedPos,
            2.2f);
    }
    else
    {
        // Horizontal slider fallback
        const float trackH = 6.0f;
        const float cy     = bounds.getCentreY();
        const juce::Rectangle<float> track(bounds.getX() + 6.0f, cy - trackH * 0.5f, bounds.getWidth() - 12.0f, trackH);

        g.setColour(kSlotRecessed);
        g.fillRect(track);
        g.setColour(kRubyBorder);
        g.drawRect(track, 1.0f);

        const float clampedX = juce::jlimit(track.getX(), track.getRight(), sliderPos);
        const float fillW    = clampedX - track.getX();
        if (fillW > 1.0f)
        {
            g.setColour(kNeonCrimson);
            g.fillRect(track.withWidth(fillW));
        }

        g.setColour(kNeonCrimsonGlow);
        g.fillEllipse(clampedX - 6.0f, cy - 6.0f, 12.0f, 12.0f);
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
    const float cutSize = 8.0f;
    auto btnPath = createChamferedPath(bounds, cutSize);

    juce::Colour fillCol = backgroundColour;
    if (shouldDrawButtonAsDown)
        fillCol = fillCol.darker(0.35f);
    else if (shouldDrawButtonAsHighlighted)
        fillCol = fillCol.brighter(0.20f);

    g.setColour(fillCol);
    g.fillPath(btnPath);

    // Resplandor neón perimetral al pasar el cursor
    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour(kNeonCrimsonGlow.withAlpha(0.35f));
        g.strokePath(btnPath, juce::PathStrokeType(3.0f));
        g.setColour(kNeonCrimsonGlow);
        g.strokePath(btnPath, juce::PathStrokeType(1.5f));
    }
    else
    {
        g.setColour(kRubyBorder);
        g.strokePath(btnPath, juce::PathStrokeType(1.0f));
    }

    // Pequeños acentos angulares en esquinas opuestas
    g.setColour(shouldDrawButtonAsHighlighted ? kNeonCyan : kNeonCrimson.withAlpha(0.6f));
    g.drawLine(bounds.getX(), bounds.getY() + cutSize + 4.0f, bounds.getX(), bounds.getY() + cutSize, 1.5f);
    g.drawLine(bounds.getRight(), bounds.getBottom() - cutSize - 4.0f, bounds.getRight(), bounds.getBottom() - cutSize, 1.5f);
}

void DashLookAndFeel::drawButtonText(
    juce::Graphics& g,
    juce::TextButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsDown);

    auto textArea = button.getLocalBounds().reduced(8, 2);
    const juce::Colour textCol = shouldDrawButtonAsHighlighted ? kNeonCyanGlow : kTextMain;

    g.setColour(textCol);
    g.setFont(getTextButtonFont(button, button.getHeight()));

    // Texto en mayúsculas técnicas
    g.drawFittedText(
        button.getButtonText().toUpperCase(),
        textArea,
        juce::Justification::centred,
        1);
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

    const float switchW = 48.0f;
    const float switchH = 22.0f;
    const float switchY = bounds.getCentreY() - switchH * 0.5f;
    const auto switchArea = juce::Rectangle<float>(bounds.getX() + 2.0f, switchY, switchW, switchH);
    auto switchPath = createChamferedPath(switchArea, 5.0f);

    if (isToggled)
    {
        juce::ColourGradient grad(
            kNeonCrimsonGlow, switchArea.getX(), switchArea.getY(),
            kMetallicRuby, switchArea.getRight(), switchArea.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(switchPath);
        g.setColour(kNeonCrimsonGlow);
        g.strokePath(switchPath, juce::PathStrokeType(1.4f));
    }
    else
    {
        g.setColour(kBgCardElevated);
        g.fillPath(switchPath);
        g.setColour(kRubyBorder);
        g.strokePath(switchPath, juce::PathStrokeType(1.0f));
    }

    const float thumbW = switchH - 4.0f;
    const float thumbH = switchH - 4.0f;
    const float thumbX = isToggled ? (switchArea.getRight() - thumbW - 2.0f) : (switchArea.getX() + 2.0f);
    const float thumbY = switchArea.getY() + 2.0f;
    const auto thumbRect = juce::Rectangle<float>(thumbX, thumbY, thumbW, thumbH);
    auto thumbPath = createChamferedPath(thumbRect, 3.0f);

    g.setColour(isToggled ? kTextMain : kNeonEmerald);
    g.fillPath(thumbPath);

    if (button.getButtonText().isNotEmpty())
    {
        const auto textArea = bounds.withTrimmedLeft(switchW + 12.0f);
        g.setColour(button.isEnabled() ? kTextMain : kTextMuted);
        g.setFont(getMonospaceFont(13.0f, isToggled));
        g.drawFittedText(button.getButtonText().toUpperCase(), textArea.toNearestInt(), juce::Justification::centredLeft, 1);
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

    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    auto boxPath = createChamferedPath(bounds, 6.0f);

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillPath(boxPath);

    g.setColour(box.hasKeyboardFocus(true) ? kNeonCrimsonGlow : box.findColour(juce::ComboBox::outlineColourId));
    g.strokePath(boxPath, juce::PathStrokeType(1.2f));

    const float arrowX = static_cast<float>(width) - 18.0f;
    const float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath(arrowX - 4.0f, arrowY - 2.0f);
    arrow.lineTo(arrowX, arrowY + 2.5f);
    arrow.lineTo(arrowX + 4.0f, arrowY - 2.0f);

    g.setColour(kNeonCyanGlow);
    g.strokePath(arrow, juce::PathStrokeType(2.0f));
}

void DashLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    drawHudPanel(g, bounds, 8.0f, true, true, kBgCardElevated, kNeonCrimson, 1.2f);
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
        auto r = area.reduced(6, 0);
        g.setColour(kRubyBorder);
        g.fillRect(r.removeFromTop(1).withY(area.getCentreY()));
        return;
    }

    if (isHighlighted && isActive)
    {
        auto itemPath = createChamferedPath(area.reduced(4, 1).toFloat(), 4.0f);
        g.setColour(kNeonCrimson.withAlpha(0.35f));
        g.fillPath(itemPath);
        g.setColour(kNeonCrimsonGlow);
        g.strokePath(itemPath, juce::PathStrokeType(1.0f));
    }

    auto textArea = area.reduced(12, 0);
    g.setColour(isHighlighted ? kNeonCyanGlow : (isActive ? kTextMain : kTextMuted));
    g.setFont(getMonospaceFont(12.5f, isHighlighted));
    g.drawFittedText(text.toUpperCase(), textArea, juce::Justification::centredLeft, 1);
}

juce::Font DashLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return getMonospaceFont(juce::jmin(13.0f, static_cast<float>(buttonHeight) * 0.44f), true);
}

juce::Font DashLookAndFeel::getLabelFont(juce::Label&)
{
    return getMonospaceFont(12.5f, false);
}

juce::Font DashLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return getMonospaceFont(12.5f, false);
}

juce::Font DashLookAndFeel::getPopupMenuFont()
{
    return getMonospaceFont(12.5f, false);
}

} // namespace dash::ui
