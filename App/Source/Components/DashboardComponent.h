#pragma once

#include <JuceHeader.h>
#include <functional>

namespace dash::ui
{

/**
 * Pantalla Principal de Control y Orquestación Táctica (DashboardComponent).
 *
 * Presenta:
 *  - Conmutador Easy Mode / Advanced Mode.
 *  - En Easy Mode: Selectores rápidos de juego, auricular y perfil con controles tácticos directos.
 *  - Cadena Visual de Procesamiento: Input -> Headroom -> Dynamics -> Transient -> Ambience -> Spatial -> EQ -> Limiter -> Output.
 *  - Estado operativo real del motor, backend, clipping y latencia.
 */
class DashboardComponent final : public juce::Component
{
public:
    enum class DisplayMode { Easy, Advanced };

    DashboardComponent();
    ~DashboardComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Callbacks para interacción hacia MainComponent
    std::function<void(DisplayMode newMode)> onDisplayModeChanged;
    std::function<void(int nodeIndex)> onChainNodeClicked;
    std::function<void()> onActivateClicked;
    std::function<void()> onDeactivateClicked;
    std::function<void(const juce::String& profileId)> onQuickProfileSelected;
    std::function<void(float gun, float stepsDb, float ambGuard, bool spatialEnabled)> onQuickControlsChanged;

    void setEngineRunning(bool running);
    void setDeviceName(const juce::String& name);
    void setFormatInfo(double sampleRate, const juce::String& layout);
    void setLatencyMs(float latencyMs);
    void setCpuPercent(float cpuPercent);

    void updateQuickControls(float gunReductionDb, float footstepsDb, float ambienceReductionDb, bool spatialEnabled);
    void getQuickControlsValues(float& outGunReductionDb, float& outFootstepsDb, float& outAmbienceReductionDb, bool& outSpatialEnabled) const;

    void setNodeBypassState(int nodeIndex, bool isBypassed);

private:
    void initTacticalSlider(juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double def, const juce::String& suffix);

    DisplayMode currentMode_ = DisplayMode::Easy;
    bool isRunning_ = false;

    // 1. Selector de modo
    juce::TextButton easyModeBtn_;
    juce::TextButton advancedModeBtn_;

    // 2. Tarjeta de Estado del Motor
    juce::Label engineCardTitle_;
    juce::Label backendLabel_;
    juce::Label deviceLabel_;
    juce::Label formatLabel_;
    juce::Label latencyLabel_;
    juce::Label cpuLabel_;
    juce::TextButton activateBtn_;
    juce::TextButton deactivateBtn_;

    // 3. Cadena Visual Interactiva (8 nodos)
    struct ChainNode
    {
        juce::String name;
        juce::String info;
        bool bypassed = false;
        juce::Rectangle<int> bounds;
    };
    std::vector<ChainNode> chainNodes_;

    // 4. Controles Tácticos Rápidos (Easy Mode)
    juce::Label quickTitleLabel_;
    juce::ComboBox quickProfileSelector_;
    juce::Slider gunSlider_;
    juce::Label  gunLabel_;
    juce::Slider stepsSlider_;
    juce::Label  stepsLabel_;
    juce::Slider ambSlider_;
    juce::Label  ambLabel_;
    juce::ToggleButton spatialToggle_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DashboardComponent)
};

} // namespace dash::ui
