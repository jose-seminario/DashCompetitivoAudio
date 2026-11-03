#pragma once

#include <JuceHeader.h>
#include <functional>

namespace dash::ui
{

/**
 * Pantalla Principal de Control y Orquestación Táctica (DashboardComponent).
 *
 * Presenta una interfaz de alto impacto visual inspirada en la estética Soft Dark / Neon Coral:
 *  - Hero Card Coral con silueta táctica, perfil activo y botón maestro.
 *  - Rack de 5 Faders Verticales en Cápsula (Disparos, Pasos, Ruido, Makeup, 3D Mix).
 *  - Tarjeta de Visualización de Forma de Onda / Espectro en líneas coral.
 *  - Cadena Interactiva de Procesamiento (9 Nodos) con micro-indicadores de bypass.
 *  - Selector rápido de perfiles en píldora y switch de Audio Espacial.
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
    std::function<void()> onAutoConfigClicked;
    std::function<void(const juce::String& profileId)> onQuickProfileSelected;
    std::function<void(float gun, float stepsDb, float ambGuard, float makeupGainDb, bool spatialEnabled)> onQuickControlsChanged;

    void setEngineRunning(bool running);
    void setDeviceName(const juce::String& name);
    void setFormatInfo(double sampleRate, const juce::String& layout);
    void setLatencyMs(float latencyMs);
    void setCpuPercent(float cpuPercent);

    void setTutorialVisible(bool visible);
    [[nodiscard]] bool isTutorialVisible() const noexcept { return showTutorial_; }

    void updateQuickControls(float gunReductionDb, float footstepsDb, float ambienceReductionDb, float makeupGainDb, bool spatialEnabled);
    void getQuickControlsValues(float& outGunReductionDb, float& outFootstepsDb, float& outAmbienceReductionDb, float& outMakeupGainDb, bool& outSpatialEnabled) const;

    void setNodeBypassState(int nodeIndex, bool isBypassed);

private:
    void initVerticalFader(juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double def, const juce::String& suffix);
    void drawHeroCard(juce::Graphics& g, const juce::Rectangle<float>& area);
    void drawWaveformCard(juce::Graphics& g, const juce::Rectangle<float>& area);

    DisplayMode currentMode_ = DisplayMode::Easy;
    bool isRunning_ = false;
    bool showTutorial_ = true;

    juce::String deviceName_ = "Altavoces de Windows";
    juce::String formatText_ = "48000 Hz | 7.1 Surround (8 canales)";
    float latencyMs_ = 0.0f;
    float cpuPercent_ = 0.0f;

    // 1. Selector de modo (Cápsulas)
    juce::TextButton easyModeBtn_;
    juce::TextButton advancedModeBtn_;

    // 2. Botones del Motor
    juce::TextButton activateBtn_;
    juce::TextButton deactivateBtn_;

    // 3. Tutorial de Primer Uso y Autoconfiguración
    juce::Label tutorialTitleLabel_;
    juce::Label step1Label_;
    juce::Label step2Label_;
    juce::Label step3Label_;
    juce::TextButton autoConfigBtn_;
    juce::TextButton dismissTutorialBtn_;

    // 4. Cadena Visual Interactiva (9 nodos)
    struct ChainNode
    {
        juce::String name;
        juce::String info;
        bool bypassed = false;
        juce::Rectangle<int> bounds;
    };
    std::vector<ChainNode> chainNodes_;

    // 5. Controles Tácticos Rápidos (Faders Verticales en Cápsula)
    juce::Label quickTitleLabel_;
    juce::ComboBox quickProfileSelector_;

    juce::Slider gunSlider_;
    juce::Label  gunLabel_;

    juce::Slider stepsSlider_;
    juce::Label  stepsLabel_;

    juce::Slider ambSlider_;
    juce::Label  ambLabel_;

    juce::Slider makeupSlider_;
    juce::Label  makeupLabel_;

    juce::Slider spatialMixSlider_;
    juce::Label  spatialMixLabel_;

    juce::ToggleButton spatialToggle_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DashboardComponent)
};

} // namespace dash::ui
