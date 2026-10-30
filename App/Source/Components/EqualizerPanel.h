#pragma once

#include <JuceHeader.h>
#include <DashAudioCore/EQProcessor.h>
#include <DashAudioCore/ParametricEQProcessor.h>

namespace dash::ui
{

/**
 * Panel de Ecualización Gráfica ISO de 31 Bandas y Paramétrica de Precisión (EqualizerPanel).
 *
 * Ofrece visualización gráfica de la curva de respuesta en frecuencia (20 Hz - 20 kHz)
 * y controles tanto para realce táctico (pasos/graves) como para bandas paramétricas individuales.
 */
class EqualizerPanel final : public juce::Component
{
public:
    EqualizerPanel();
    ~EqualizerPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setParameters(
        const dash::core::EQParameters& graphicEq,
        bool useParametric,
        const std::array<dash::core::ParametricBandParameters, dash::core::ParametricEQProcessor::kMaxBands>& bands);

    std::function<void(
        const dash::core::EQParameters&,
        bool,
        const std::array<dash::core::ParametricBandParameters, dash::core::ParametricEQProcessor::kMaxBands>&)> onParametersChanged;

private:
    void drawFrequencyResponseCurve(juce::Graphics& g, const juce::Rectangle<float>& area);
    void updateCurrentBandControls();
    void notifyChange();

    dash::core::EQParameters graphicParams_;
    bool useParametric_ = false;
    std::array<dash::core::ParametricBandParameters, dash::core::ParametricEQProcessor::kMaxBands> bands_ {};
    int selectedBandIndex_ = 0;

    juce::Label titleLabel_;
    juce::ToggleButton bypassToggle_;
    juce::ToggleButton parametricModeToggle_;

    // Controles tácticos rápidos
    juce::Slider footstepsSlider_;
    juce::Label  footstepsLabel_;
    juce::Slider bassSlider_;
    juce::Label  bassLabel_;
    juce::Slider preampSlider_;
    juce::Label  preampLabel_;

    // Editor de banda paramétrica seleccionada
    juce::Label bandSelectLabel_;
    juce::ComboBox bandSelector_;
    juce::ToggleButton bandEnableToggle_;

    juce::Label filterTypeLabel_;
    juce::ComboBox filterTypeSelector_;

    juce::Slider freqSlider_;
    juce::Label  freqLabel_;
    juce::Slider gainSlider_;
    juce::Label  gainLabel_;
    juce::Slider qSlider_;
    juce::Label  qLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerPanel)
};

} // namespace dash::ui
