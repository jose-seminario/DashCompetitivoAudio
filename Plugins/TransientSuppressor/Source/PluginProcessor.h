#pragma once

#include <JuceHeader.h>
#include <atomic>

#include "DSP/CenterTransientSuppressor.h"

class TransientSuppressorAudioProcessor final : public juce::AudioProcessor
{
public:
    struct ParamIDs
    {
        static constexpr const char* gun     = "Gun";
        static constexpr const char* steps   = "Steps";
        static constexpr const char* front   = "Front";
        static constexpr const char* restore = "Restore";
        static constexpr const char* release = "Release";
        static constexpr const char* bypass  = "Bypass";
    };

    TransientSuppressorAudioProcessor();
    ~TransientSuppressorAudioProcessor() override = default;

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
    const juce::String getProgramName(int) override            { return "Campaign Gunshot Guard (Gun 0.45)"; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts_; }

    [[nodiscard]] float getMeterReductionDb() const noexcept  { return meterReductionDb_.load(std::memory_order_relaxed); }
    [[nodiscard]] int   getActiveChannelCount() const noexcept { return activeChannels_.load(std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts_;
    dash::dsp::CenterTransientSuppressor suppressor_;

    // Punteros atómicos cacheados para acceso O(1) libre de bloqueos en el hilo de audio
    std::atomic<float>* gunParam_     = nullptr;
    std::atomic<float>* stepsParam_   = nullptr;
    std::atomic<float>* frontParam_   = nullptr;
    std::atomic<float>* restoreParam_ = nullptr;
    std::atomic<float>* releaseParam_ = nullptr;
    std::atomic<float>* bypassParam_  = nullptr;

    std::atomic<float> meterReductionDb_ { 0.0f };
    std::atomic<int>   activeChannels_   { 2 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientSuppressorAudioProcessor)
};
