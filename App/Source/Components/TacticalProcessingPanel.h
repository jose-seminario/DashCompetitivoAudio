#pragma once

#include <JuceHeader.h>
#include <DashAudioCore/TransientProcessor.h>
#include <DashAudioCore/AmbienceProcessor.h>
#include <DashAudioCore/LimiterProcessor.h>

namespace dash::ui
{

/**
 * Panel de Procesamiento Táctico Extremo (TacticalProcessingPanel).
 *
 * Controla:
 *  1. Transient Suppressor (Supresor de Gun Crack y explosiones con crossover Linkwitz-Riley).
 *  2. Ambience Guard (Filtro protector de ruido ambiental multibanda 350 Hz - 2.5 kHz).
 *  3. Limiter Processor (Limitador acústico de seguridad de pico absoluto).
 */
class TacticalProcessingPanel final : public juce::Component
{
public:
    TacticalProcessingPanel();
    ~TacticalProcessingPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setParameters(
        const dash::core::TransientParameters& trans,
        const dash::core::AmbienceParameters& amb,
        const dash::core::LimiterParameters& lim);

    std::function<void(
        const dash::core::TransientParameters&,
        const dash::core::AmbienceParameters&,
        const dash::core::LimiterParameters&)> onParametersChanged;

private:
    void initKnob(juce::Slider& s, juce::Label& l, const juce::String& text, double min, double max, double def, const juce::String& suffix);
    void notifyChange();

    // 1. Transient Suppressor
    juce::Label transientHeader_;
    juce::ToggleButton transientBypass_;
    juce::Slider gunReductionSlider_;
    juce::Label  gunReductionLabel_;
    juce::Slider gunCrossoverSlider_;
    juce::Label  gunCrossoverLabel_;
    juce::Slider gunAttackSlider_;
    juce::Label  gunAttackLabel_;
    juce::Slider gunReleaseSlider_;
    juce::Label  gunReleaseLabel_;

    // 2. Ambience Guard
    juce::Label ambHeader_;
    juce::ToggleButton ambBypass_;
    juce::Slider ambReductionSlider_;
    juce::Label  ambReductionLabel_;
    juce::Slider ambLowCutSlider_;
    juce::Label  ambLowCutLabel_;
    juce::Slider ambHighCutSlider_;
    juce::Label  ambHighCutLabel_;

    // 3. Limiter Processor
    juce::Label limHeader_;
    juce::ToggleButton limBypass_;
    juce::Slider limCeilingSlider_;
    juce::Label  limCeilingLabel_;
    juce::Slider limReleaseSlider_;
    juce::Label  limReleaseLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TacticalProcessingPanel)
};

} // namespace dash::ui
