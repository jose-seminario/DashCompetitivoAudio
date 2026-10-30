#pragma once

#include <JuceHeader.h>
#include <atomic>

#include "DSP/HybridLoudnessCompressor.h"

class LoudnessCompressorAudioProcessor final : public juce::AudioProcessor
{
public:
    struct ParamIDs
    {
        static constexpr const char* amount  = "Amount";
        static constexpr const char* target  = "Target";
        static constexpr const char* release = "Release";
        static constexpr const char* detRel  = "DetRel";
        static constexpr const char* limRel  = "LimRel";
        static constexpr const char* scLow   = "SCLow";
        static constexpr const char* scHigh  = "SCHigh";
        static constexpr const char* lift    = "Lift";
        static constexpr const char* gate    = "Gate";
        static constexpr const char* bypass  = "Bypass";
    };

    LoudnessCompressorAudioProcessor();
    ~LoudnessCompressorAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram(int) override                       {}
    const juce::String getProgramName(int) override            { return "Competitive Default (-18 LUFS)"; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts_; }

    [[nodiscard]] float getMeterLufs() const noexcept   { return meterLufs_.load(std::memory_order_relaxed); }
    [[nodiscard]] float getMeterGainDb() const noexcept { return meterGainDb_.load(std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts_;
    dash::dsp::HybridLoudnessCompressor compressor_;

    // Punteros atómicos cacheados para lectura O(1) sin bloqueos en processBlock()
    std::atomic<float>* amountParam_  = nullptr;
    std::atomic<float>* targetParam_  = nullptr;
    std::atomic<float>* releaseParam_ = nullptr;
    std::atomic<float>* detRelParam_  = nullptr;
    std::atomic<float>* limRelParam_  = nullptr;
    std::atomic<float>* scLowParam_   = nullptr;
    std::atomic<float>* scHighParam_  = nullptr;
    std::atomic<float>* liftParam_    = nullptr;
    std::atomic<float>* gateParam_    = nullptr;
    std::atomic<float>* bypassParam_  = nullptr;

    // Telemetría atómica hacia la interfaz gráfica (UI)
    std::atomic<float> meterLufs_   { -70.0f };
    std::atomic<float> meterGainDb_ { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoudnessCompressorAudioProcessor)
};
