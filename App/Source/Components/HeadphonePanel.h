#pragma once

#include <JuceHeader.h>
#include <DashAudioCore/ParametricEQProcessor.h>
#include <functional>
#include <vector>

namespace dash::ui
{

/**
 * Panel de Calibración Acústica por Auricular (HeadphonePanel).
 *
 * Muestra el estado de calibración verificada (AutoEq hacia curva Harman 2018)
 * y transmite los filtros paramétricos de compensación al AudioGraph.
 */
class HeadphonePanel final : public juce::Component
{
public:
    HeadphonePanel();
    ~HeadphonePanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(const juce::String& headphoneModelId)> onHeadphoneModelChanged;
    std::function<void(bool applyCompensation, const std::vector<dash::core::ParametricBandParameters>& bands)> onCompensationChanged;

    [[nodiscard]] bool isCompensationEnabled() const noexcept { return enableCompensationToggle_.getToggleState(); }
    [[nodiscard]] std::vector<dash::core::ParametricBandParameters> getSelectedHeadphoneBands() const;
    void setSelectedModelId(const juce::String& modelId);
    void setCompensationEnabled(bool enabled);

private:
    struct HeadphoneProfileItem
    {
        juce::String id;
        juce::String displayName;
        juce::String manufacturer;
        juce::String model;
        juce::String statusBadge;
        juce::String targetCurve;
        juce::String measurementSource;
        juce::String license;
        juce::String description;
        bool isCalibrated = false;
        std::vector<dash::core::ParametricBandParameters> bands;
    };

    void scanHeadphoneProfiles();
    void updateModelInfo();

    std::vector<HeadphoneProfileItem> headphoneItems_;

    juce::Label titleLabel_;
    juce::Label modelSelectLabel_;
    juce::ComboBox modelSelector_;

    juce::Label statusBadgeLabel_;
    juce::Label manufacturerLabel_;
    juce::Label targetCurveLabel_;
    juce::Label measurementSourceLabel_;
    juce::Label licenseLabel_;
    juce::Label descriptionLabel_;

    juce::ToggleButton enableCompensationToggle_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeadphonePanel)
};

} // namespace dash::ui
