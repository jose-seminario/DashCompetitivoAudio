#include "DashLookAndFeel.h"

#include <cmath>

namespace dash::ui
{

DashLookAndFeel::DashLookAndFeel()
{
    setDefaultSansSerifTypefaceName("Segoe UI");

    setColour(juce::ResizableWindow::backgroundColourId, kBgDark);
    setColour(juce::TabbedComponent::backgroundColourId, kBgDark);
    setColour(juce::TabbedComponent::outlineColourId,    juce::Colours::transparentBlack);

    setColour(juce::ComboBox::backgroundColourId,        kBgCardElevated);
    setColour(juce::ComboBox::outlineColourId,           kBorderSubtle);
    setColour(juce::ComboBox::textColourId,              kTextMain);
    setColour(juce::ComboBox::arrowColourId,             kAccentCyanGlow);

    setColour(juce::PopupMenu::backgroundColourId,            kBgCardElevated);
    setColour(juce::PopupMenu::textColourId,                 kTextMain);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kBorderSubtle);
    setColour(juce::PopupMenu::highlightedTextColourId,       kAccentCyanGlow);

    setColour(juce::TextButton::buttonColourId,   kBgCardElevated);
    setColour(juce::TextButton::textColourOffId,  kTextMain);
    setColour(juce::TextButton::textColourOnId,   kAccentCyanGlow);

    setColour(juce::Label::textColourId, kTextMain);
    setColour(juce::Slider::textBoxTextColourId,       kTextMain);
    setColour(juce::Slider::textBoxBackgroundColourId, kSlotRecessed);
    setColour(juce::Slider::textBoxOutlineColourId,    kBorderSubtle);
    setColour(juce::Slider::trackColourId,             kAccentCyan);
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
        const float padTop    = 12.0f;
        const float padBottom = 12.0f;
        const float slotWidth = juce::jlimit(10.0f, 16.0f, bounds.getWidth() * 0.18f);
        const float centreX   = bounds.getCentreX();
        const float slotTop   = bounds.getY() + padTop;
        const float slotH     = juce::jmax(24.0f, bounds.getHeight() - padTop - padBottom);
        const float slotBot   = slotTop + slotH;

        const juce::Rectangle<float> slotBounds(
            centreX - slotWidth * 0.5f,
            slotTop,
            slotWidth,
            slotH);

        // 1. Marcas de nivel estilo consola de audio (-18 a +18 dB en pasos de 3 dB)
        constexpr int kNumTicks = 13; // +18, +15, +12, +9, +6, +3, 0, -3, -6, -9, -12, -15, -18
        const float tickInnerGap = slotWidth * 0.5f + 5.0f;

        for (int i = 0; i < kNumTicks; ++i)
        {
            const float norm = static_cast<float>(i) / static_cast<float>(kNumTicks - 1);
            const float ty   = slotTop + norm * slotH;
            const bool isZeroCenter = (i == (kNumTicks / 2));
            const bool isMajor      = ((i % 2) == 0);

            const float tickLen = isZeroCenter ? 11.0f : (isMajor ? 7.5f : 4.5f);
            const float strokeW = isZeroCenter ? 1.8f : 1.0f;

            g.setColour(isZeroCenter ? kAccentCyanGlow.withAlpha(0.85f)
                                     : kTextMuted.withAlpha(isMajor ? 0.55f : 0.30f));

            g.drawLine(centreX - tickInnerGap - tickLen, ty, centreX - tickInnerGap, ty, strokeW);
            g.drawLine(centreX + tickInnerGap, ty, centreX + tickInnerGap + tickLen, ty, strokeW);
        }

        // 2. Fondo oscuro hundido del carril del fader
        g.setColour(kSlotRecessed);
        g.fillRoundedRectangle(slotBounds, slotWidth * 0.5f);
        g.setColour(kBorderSubtle);
        g.drawRoundedRectangle(slotBounds, slotWidth * 0.5f, 1.2f);

        // 3. Barra de color neón activa
        const float clampedPos = juce::jlimit(slotTop, slotBot, sliderPos);
        const juce::Colour accentBase = slider.findColour(juce::Slider::trackColourId, true);
        const bool isBipolar = (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0);

        if (slider.isEnabled())
        {
            juce::Path clipPath;
            clipPath.addRoundedRectangle(slotBounds.reduced(1.5f), (slotWidth - 3.0f) * 0.5f);
            juce::Graphics::ScopedSaveState clipState(g);
            g.reduceClipRegion(clipPath);

            if (isBipolar)
            {
                const float zeroY = slotTop + slotH * 0.5f;
                const float topY  = juce::jmin(zeroY, clampedPos);
                const float botY  = juce::jmax(zeroY, clampedPos);
                const float barH  = botY - topY;

                // Relleno tenue de referencia bajo el fader
                g.setColour(accentBase.withAlpha(0.14f));
                g.fillRect(slotBounds.getX(), clampedPos, slotWidth, slotBot - clampedPos);

                if (barH > 0.8f)
                {
                    const bool isBoost = (clampedPos < zeroY);
                    const juce::Colour cTop = isBoost ? kAccentCyanGlow : accentBase;
                    const juce::Colour cBot = isBoost ? accentBase      : kAccentCoral;

                    juce::ColourGradient neonGrad(
                        cTop, centreX, topY,
                        cBot, centreX, botY,
                        false);
                    g.setGradientFill(neonGrad);
                    g.fillRect(slotBounds.getX(), topY, slotWidth, barH);
                }
            }
            else
            {
                const float barH = slotBot - clampedPos;
                if (barH > 0.8f)
                {
                    juce::ColourGradient neonGrad(
                        kAccentCyanGlow, centreX, clampedPos,
                        accentBase,      centreX, slotBot,
                        false);
                    g.setGradientFill(neonGrad);
                    g.fillRect(slotBounds.getX(), clampedPos, slotWidth, barH);
                }
            }
        }

        // 4. Capuchón rectangular de fader estilo consola de mezcla profesional
        const float capW = juce::jlimit(26.0f, 40.0f, bounds.getWidth() * 0.52f);
        const float capH = 18.0f;
        const juce::Rectangle<float> capBounds(
            centreX - capW * 0.5f,
            clampedPos - capH * 0.5f,
            capW,
            capH);

        // Sombra proyectada
        g.setColour(juce::Colours::black.withAlpha(0.50f));
        g.fillRoundedRectangle(capBounds.translated(0.0f, 2.5f), 4.0f);

        // Cuerpo metálico oscuro del capuchón
        juce::ColourGradient capGrad(
            juce::Colour(0xff343b4d), centreX, capBounds.getY(),
            juce::Colour(0xff1d222d), centreX, capBounds.getBottom(),
            false);
        g.setGradientFill(capGrad);
        g.fillRoundedRectangle(capBounds, 4.0f);

        // Borde iluminado y línea indicadora central en neón
        const juce::Colour lineCol = slider.isEnabled() ? kAccentCyanGlow : kTextMuted;
        g.setColour(slider.isEnabled() ? accentBase.withAlpha(0.85f) : kBorderSubtle);
        g.drawRoundedRectangle(capBounds, 4.0f, 1.3f);

        g.setColour(lineCol);
        g.drawLine(
            capBounds.getX() + 4.0f,
            clampedPos,
            capBounds.getRight() - 4.0f,
            clampedPos,
            2.2f);
    }
    else
    {
        const float trackH = 8.0f;
        const float cy     = bounds.getCentreY();
        const juce::Rectangle<float> track(bounds.getX() + 6.0f, cy - trackH * 0.5f, bounds.getWidth() - 12.0f, trackH);

        g.setColour(kSlotRecessed);
        g.fillRoundedRectangle(track, trackH * 0.5f);
        g.setColour(kBorderSubtle);
        g.drawRoundedRectangle(track, trackH * 0.5f, 1.0f);

        const float clampedX = juce::jlimit(track.getX(), track.getRight(), sliderPos);
        const float fillW    = clampedX - track.getX();
        if (fillW > 1.0f)
        {
            g.setColour(kAccentCyan);
            g.fillRoundedRectangle(track.withWidth(fillW), trackH * 0.5f);
        }

        g.setColour(kAccentCyanGlow);
        g.fillEllipse(clampedX - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    }
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

    const float switchW = 46.0f;
    const float switchH = 24.0f;
    const float switchY = bounds.getCentreY() - switchH * 0.5f;
    const auto switchArea = juce::Rectangle<float>(bounds.getX() + 2.0f, switchY, switchW, switchH);
    const float cornerRadius = switchH * 0.5f;

    if (isToggled)
    {
        juce::ColourGradient grad(
            kAccentCoralLight, switchArea.getX(), switchArea.getY(),
            kAccentCoral, switchArea.getRight(), switchArea.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(switchArea, cornerRadius);
        g.setColour(kAccentCoralLight.withAlpha(0.55f));
        g.drawRoundedRectangle(switchArea, cornerRadius, 1.2f);
    }
    else
    {
        g.setColour(kBgCardElevated);
        g.fillRoundedRectangle(switchArea, cornerRadius);
        g.setColour(kBorderSubtle);
        g.drawRoundedRectangle(switchArea, cornerRadius, 1.2f);
    }

    const float thumbSize = switchH - 6.0f;
    const float thumbX = isToggled ? (switchArea.getRight() - thumbSize - 3.0f) : (switchArea.getX() + 3.0f);
    const float thumbY = switchArea.getY() + 3.0f;

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(thumbX, thumbY + 1.2f, thumbSize, thumbSize);

    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    if (button.getButtonText().isNotEmpty())
    {
        const auto textArea = bounds.withTrimmedLeft(switchW + 12.0f);
        g.setColour(button.isEnabled() ? kTextMain : kTextMuted);
        g.setFont(juce::Font(juce::FontOptions()
            .withName("Segoe UI")
            .withHeight(14.0f)
            .withStyle(isToggled ? "Bold" : "Plain")));
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
    const float cornerRadius = 6.0f;

    juce::Colour fillCol = backgroundColour;
    if (shouldDrawButtonAsDown)
        fillCol = fillCol.darker(0.18f);
    else if (shouldDrawButtonAsHighlighted)
        fillCol = fillCol.brighter(0.10f);

    g.setColour(fillCol);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour((shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) ? kAccentCyanGlow : kBorderSubtle);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.2f);
    juce::ignoreUnused(button);
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
    const float cornerRadius = 6.0f;

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(box.hasKeyboardFocus(true) ? kBorderFocus : box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.2f);

    const float arrowX = static_cast<float>(width) - 18.0f;
    const float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath(arrowX - 4.5f, arrowY - 2.5f);
    arrow.lineTo(arrowX, arrowY + 2.5f);
    arrow.lineTo(arrowX + 4.5f, arrowY - 2.5f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(arrow, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void DashLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
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
        auto r = area.reduced(6, 0);
        g.setColour(kBorderSubtle);
        g.fillRect(r.removeFromTop(1).withY(area.getCentreY()));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(kBorderSubtle);
        g.fillRoundedRectangle(area.reduced(4, 1).toFloat(), 4.0f);
    }

    auto textArea = area.reduced(12, 0);
    g.setColour(isHighlighted ? kAccentCyanGlow : (isActive ? kTextMain : kTextMuted));
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.5f)));
    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
}

void DashLookAndFeel::drawTabButton(
    juce::TabBarButton& button,
    juce::Graphics& g,
    bool isMouseOver,
    bool isMouseDown)
{
    juce::ignoreUnused(isMouseDown);

    const auto activeArea = button.getActiveArea().toFloat();
    const bool isFront = button.isFrontTab();

    g.setColour(isFront ? kBgCard : kBgDark);
    g.fillRect(activeArea);

    if (isFront)
    {
        g.setColour(kAccentCyanGlow);
        g.fillRect(activeArea.getX() + 10.0f, activeArea.getBottom() - 3.0f, activeArea.getWidth() - 20.0f, 3.0f);
    }
    else if (isMouseOver)
    {
        g.setColour(kBgCardElevated.withAlpha(0.55f));
        g.fillRect(activeArea);
    }

    g.setColour(isFront ? kTextMain : (isMouseOver ? kTextMain.withAlpha(0.85f) : kTextMuted));
    g.setFont(getTabButtonFont(button, activeArea.getHeight()));
    g.drawFittedText(
        button.getButtonText(),
        button.getTextArea(),
        juce::Justification::centred,
        1);
}

int DashLookAndFeel::getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth)
{
    const auto font = getTabButtonFont(button, static_cast<float>(tabDepth));
    juce::GlyphArrangement ga;
    ga.addLineOfText(font, button.getButtonText(), 0.0f, 0.0f);
    const int textW = static_cast<int>(std::ceil(ga.getBoundingBox(0, -1, true).getWidth()));
    return juce::jmax(170, textW + 56);
}

void DashLookAndFeel::drawTabAreaBehindFrontButton(
    juce::TabbedButtonBar& /*bar*/,
    juce::Graphics& g,
    int w,
    int h)
{
    g.setColour(kBorderSubtle);
    g.fillRect(0, h - 1, w, 1);
}

juce::Font DashLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions()
        .withName("Segoe UI")
        .withHeight(juce::jmin(13.5f, static_cast<float>(buttonHeight) * 0.48f))
        .withStyle("Bold"));
}

juce::Font DashLookAndFeel::getTabButtonFont(juce::TabBarButton&, float /*height*/)
{
    return juce::Font(juce::FontOptions()
        .withName("Segoe UI")
        .withHeight(14.0f)
        .withStyle("Bold"));
}

juce::Font DashLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.0f));
}

juce::Font DashLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.5f));
}

juce::Font DashLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(13.5f));
}

} // namespace dash::ui
