#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>

namespace dash::ui
{

/**
 * Barra Lateral Táctica Izquierda (SidebarComponent).
 *
 * Proporciona:
 *  - Logotipo y branding superior de Dash Competitivo Audio.
 *  - Navegación vertical fluida mediante botones tipo píldora (pill items).
 *  - Tarjeta inferior de telemetría y estado de motor WASAPI en tiempo real.
 */
class SidebarComponent final : public juce::Component
{
public:
    SidebarComponent();
    ~SidebarComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int index)> onItemSelected;

    void setSelectedItem(int index);
    [[nodiscard]] int getSelectedItem() const noexcept { return selectedIndex_; }

    void setEngineStatus(bool isRunning, float latencyMs, float cpuPercent);

private:
    struct NavItem
    {
        juce::String name;
        juce::String tag;
        std::unique_ptr<juce::TextButton> button;
    };

    void drawBrandingHeader(juce::Graphics& g, const juce::Rectangle<float>& area);
    void drawTelemetryCard(juce::Graphics& g, const juce::Rectangle<float>& area);

    int selectedIndex_ = 0;
    bool isRunning_ = false;
    float latencyMs_ = 0.0f;
    float cpuPercent_ = 0.0f;

    std::vector<NavItem> navItems_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarComponent)
};

} // namespace dash::ui
