#pragma once

#include <JuceHeader.h>

namespace dash::ui
{

/**
 * Panel de Rendimiento, Latencia y Benchmark DSP (BenchmarkPanel).
 *
 * Muestra el desglose de tiempo de procesamiento por etapa del AudioGraph
 * y comprueba el margen de seguridad respecto al presupuesto de tiempo del búfer.
 */
class BenchmarkPanel final : public juce::Component
{
public:
    BenchmarkPanel();
    ~BenchmarkPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void runBenchmarkTest();

    juce::Label titleLabel_;
    juce::Label summaryBudgetLabel_;
    juce::TextButton runBenchmarkBtn_;

    struct BenchmarkMetric
    {
        juce::String stageName;
        float executionTimeUs;
        float cpuPercent;
        juce::String status;
    };

    std::vector<BenchmarkMetric> metrics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BenchmarkPanel)
};

} // namespace dash::ui
