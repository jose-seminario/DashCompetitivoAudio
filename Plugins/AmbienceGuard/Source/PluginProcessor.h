#pragma once

#include <JuceHeader.h>
#include <atomic>

#include <DashAudioCore/AmbienceProcessor.h>

class AmbienceGuardAudioProcessor final : public juce::AudioProcessor
{
public:
    struct ParamIDs
    {
        static constexpr const char* ambGuar = "AmbGuar";
        static constexpr const char* distant = "Distant";
        static constexpr const char* target  = "Target";
        static constexpr const char* release = "Release";
        static constexpr const char* detRel  = "DetRel";
        static constexpr const char* scLow   = "SCLow";
        static constexpr const char* scHigh  = "SCHigh";
        static constexpr const char* bandMod = "BandMod";
        static constexpr const char* bypass  = "Bypass";
    };

    AmbienceGuardAudioProcessor();
    ~AmbienceGuardAudioProcessor() override = default;

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
    const juce::String getProgramName(int) override            { return "Tactical Ambience Guard (350Hz-2.5kHz)"; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts_; }

    [[nodiscard]] float getMeterHpHz() const noexcept   { return meterHpHz_.load(std::memory_order_relaxed); }
    [[nodiscard]] float getMeterLpHz() const noexcept   { return meterLpHz_.load(std::memory_order_relaxed); }
    [[nodiscard]] float getMeterGainDb() const noexcept { return meterGainDb_.load(std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts_;
    dash::core::AmbienceProcessor guard_;

    // Punteros atómicos cacheados para acceso libre de bloqueos en el hilo de audio
    std::atomic<float>* ambGuarParam_ = nullptr;
    std::atomic<float>* distantParam_ = nullptr;
    std::atomic<float>* targetParam_  = nullptr;
    std::atomic<float>* releaseParam_ = nullptr;
    std::atomic<float>* detRelParam_  = nullptr;
    std::atomic<float>* scLowParam_   = nullptr;
    std::atomic<float>* scHighParam_  = nullptr;
    std::atomic<float>* bandModParam_ = nullptr;
    std::atomic<float>* bypassParam_  = nullptr;

    std::atomic<float> meterHpHz_   { 350.0f };
    std::atomic<float> meterLpHz_   { 2500.0f };
    std::atomic<float> meterGainDb_ { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmbienceGuardAudioProcessor)
};
