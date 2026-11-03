#include "SidebarComponent.h"
#include "../Theme/DashLookAndFeel.h"

namespace dash::ui
{

SidebarComponent::SidebarComponent()
{
    const std::vector<std::pair<juce::String, juce::String>> items = {
        { "INICIO",      "Dashboard" },
        { "PERFIL",      "Presets JSON" },
        { "MOTOR",       "WASAPI & Dev" },
        { juce::String::fromUTF8(u8"DINÁMICA"),   "ITU BS.1770" },
        { juce::String::fromUTF8(u8"TÁCTICO"),    "Disparos & EQ" },
        { "EQ",          "ISO 31 & PEQ" },
        { "3D ESPACIAL", "HRTF & Cross" },
        { "AURICULARES", "Calibración" },
        { "BENCHMARK",   "Latencia & FFT" },
        { "SISTEMA",     "Diagnóstico" }
    };

    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        NavItem item;
        item.name = items[static_cast<size_t>(i)].first;
        item.tag  = items[static_cast<size_t>(i)].second;
        item.button = std::make_unique<juce::TextButton>(item.name);
        item.button->setConnectedEdges(0);

        const int targetIndex = i;
        item.button->onClick = [this, targetIndex] {
            setSelectedItem(targetIndex);
            if (onItemSelected)
                onItemSelected(targetIndex);
        };

        addAndMakeVisible(*item.button);
        navItems_.push_back(std::move(item));
    }

    setSelectedItem(0);
}

void SidebarComponent::setSelectedItem(int index)
{
    selectedIndex_ = juce::jlimit(0, static_cast<int>(navItems_.size()) - 1, index);

    for (int i = 0; i < static_cast<int>(navItems_.size()); ++i)
    {
        auto& btn = *navItems_[static_cast<size_t>(i)].button;
        if (i == selectedIndex_)
        {
            btn.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kAccentCoral);
            btn.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kPillWhite);
        }
        else
        {
            btn.setColour(juce::TextButton::buttonColourId, DashLookAndFeel::kBgCardElevated);
            btn.setColour(juce::TextButton::textColourOffId, DashLookAndFeel::kTextMuted);
        }
    }
    repaint();
}

void SidebarComponent::setEngineStatus(bool isRunning, float latencyMs, float cpuPercent)
{
    isRunning_  = isRunning;
    latencyMs_  = latencyMs;
    cpuPercent_ = cpuPercent;
    repaint();
}

void SidebarComponent::paint(juce::Graphics& g)
{
    // Fondo de barra lateral (Tarjetas oscuras / Charcoal de la referencia)
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(DashLookAndFeel::kBgCard);
    g.fillRect(bounds);

    // Borde divisor derecho sutil
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawVerticalLine(getWidth() - 1, 0.0f, (float)getHeight());

    // 1. Branding Header Superior
    drawBrandingHeader(g, juce::Rectangle<float>(14.0f, 16.0f, (float)getWidth() - 28.0f, 54.0f));

    // 2. Tarjeta Inferior de Telemetría
    const float cardH = 74.0f;
    drawTelemetryCard(g, juce::Rectangle<float>(12.0f, (float)getHeight() - cardH - 12.0f, (float)getWidth() - 24.0f, cardH));
}

void SidebarComponent::drawBrandingHeader(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    // Emblema circular táctico con calavera/nodo neón coral
    const float emblemSize = 34.0f;
    const auto emblemBounds = juce::Rectangle<float>(area.getX(), area.getY() + 4.0f, emblemSize, emblemSize);

    juce::ColourGradient emblemGrad(
        DashLookAndFeel::kAccentCoralLight, emblemBounds.getX(), emblemBounds.getY(),
        DashLookAndFeel::kAccentCoral, emblemBounds.getRight(), emblemBounds.getBottom(), false);
    g.setGradientFill(emblemGrad);
    g.fillEllipse(emblemBounds);

    // Calavera/punto de pulso central en blanco
    g.setColour(DashLookAndFeel::kPillWhite);
    g.fillEllipse(emblemBounds.getCentreX() - 4.0f, emblemBounds.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour(DashLookAndFeel::kPillWhite.withAlpha(0.6f));
    g.drawEllipse(emblemBounds.getCentreX() - 8.0f, emblemBounds.getCentreY() - 8.0f, 16.0f, 16.0f, 1.2f);

    // Texto de Branding
    const float textX = emblemBounds.getRight() + 10.0f;
    g.setColour(DashLookAndFeel::kTextMain);
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(15.5f).withStyle("Bold")));
    g.drawText("DASH AUDIO", (int)textX, (int)area.getY() + 2, (int)area.getWidth() - (int)emblemSize - 10, 18, juce::Justification::centredLeft);

    // Badge "COMPETITIVO"
    const auto badgeBounds = juce::Rectangle<float>(textX, area.getY() + 22.0f, 88.0f, 16.0f);
    g.setColour(DashLookAndFeel::kBgCardElevated);
    g.fillRoundedRectangle(badgeBounds, 8.0f);
    g.setColour(DashLookAndFeel::kAccentCoralLight.withAlpha(0.5f));
    g.drawRoundedRectangle(badgeBounds, 8.0f, 1.0f);

    g.setColour(DashLookAndFeel::kAccentCoralLight);
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(9.5f).withStyle("Bold")));
    g.drawText("COMPETITIVO", badgeBounds.toNearestInt(), juce::Justification::centred);
}

void SidebarComponent::drawTelemetryCard(juce::Graphics& g, const juce::Rectangle<float>& area)
{
    // Tarjeta encapsulada elevada
    g.setColour(DashLookAndFeel::kBgCardElevated);
    g.fillRoundedRectangle(area, 12.0f);
    g.setColour(DashLookAndFeel::kBorderSubtle);
    g.drawRoundedRectangle(area, 12.0f, 1.0f);

    // Estado del Motor con LED de pulso
    const float ledX = area.getX() + 12.0f;
    const float ledY = area.getY() + 16.0f;
    g.setColour(isRunning_ ? DashLookAndFeel::kSuccessEmerald : DashLookAndFeel::kWarningAmber);
    g.fillEllipse(ledX, ledY - 4.0f, 8.0f, 8.0f);

    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(11.5f).withStyle("Bold")));
    g.setColour(DashLookAndFeel::kTextMain);
    g.drawText(isRunning_ ? "MOTOR ACTIVO" : "MOTOR EN ESPERA", (int)ledX + 14, (int)area.getY() + 8, (int)area.getWidth() - 36, 16, juce::Justification::centredLeft);

    // Píldoras de telemetría (Latencia y CPU)
    const float pillY = area.getY() + 32.0f;
    const float pillW = (area.getWidth() - 28.0f) * 0.5f;
    const float pillH = 22.0f;

    // Píldora Latencia
    const auto latArea = juce::Rectangle<float>(area.getX() + 10.0f, pillY, pillW, pillH);
    g.setColour(DashLookAndFeel::kBgDark);
    g.fillRoundedRectangle(latArea, 11.0f);
    g.setFont(juce::Font(juce::FontOptions().withName("Segoe UI").withHeight(10.0f).withStyle("Bold")));
    g.setColour(DashLookAndFeel::kAccentCyan);
    juce::String latStr = (latencyMs_ > 0.0f) ? juce::String(latencyMs_, 1) + " ms" : "-- ms";
    g.drawText(latStr, latArea.toNearestInt(), juce::Justification::centred);

    // Píldora CPU
    const auto cpuArea = juce::Rectangle<float>(latArea.getRight() + 8.0f, pillY, pillW, pillH);
    g.setColour(DashLookAndFeel::kBgDark);
    g.fillRoundedRectangle(cpuArea, 11.0f);
    g.setColour(DashLookAndFeel::kAccentCoralLight);
    juce::String cpuStr = juce::String(cpuPercent_, 1) + "%";
    g.drawText(cpuStr, cpuArea.toNearestInt(), juce::Justification::centred);
}

void SidebarComponent::resized()
{
    const int startY = 82;
    const int itemH  = 34;
    const int gap    = 6;
    const int margin = 10;
    const int itemW  = getWidth() - margin * 2;

    for (size_t i = 0; i < navItems_.size(); ++i)
    {
        navItems_[i].button->setBounds(margin, startY + static_cast<int>(i) * (itemH + gap), itemW, itemH);
    }
}

} // namespace dash::ui
