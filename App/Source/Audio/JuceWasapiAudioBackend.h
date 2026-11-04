#pragma once

#include "IAudioBackend.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace dash::audio
{

/**
 * Backend de audio estéreo nativo de Windows basado en WASAPI sobre juce::AudioDeviceManager.
 * Procesa exclusivamente hasta 2 canales estéreo en tiempo real.
 */
class JuceWasapiAudioBackend final
    : public IAudioBackend,
      private juce::AudioIODeviceCallback
{
public:
    JuceWasapiAudioBackend();
    ~JuceWasapiAudioBackend() override;

    bool initialize(std::string* outError = nullptr) override;
    void shutdown() override;

    std::vector<AudioDeviceInfo> enumerateRenderDevices(std::string* outError = nullptr) override;
    std::vector<AudioDeviceInfo> enumerateCaptureDevices(std::string* outError = nullptr) override;

    bool start(
        const AudioBackendConfig& config,
        AudioProcessCallback processCallback,
        AudioStateCallback stateCallback,
        std::string* outError = nullptr) override;

    bool stop(std::string* outError = nullptr) override;

    [[nodiscard]] AudioBackendState getState() const noexcept override;
    void setBypass(bool bypassed) noexcept override;
    [[nodiscard]] bool isBypassed() const noexcept override;
    [[nodiscard]] std::string getLastError() const override;

    [[nodiscard]] float getCurrentLatencyMs() const noexcept;
    [[nodiscard]] float getCpuUsage() const noexcept;

private:
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String& errorMessage) override;

    static constexpr int kMaxIntermediateChannels = 2;
    juce::AudioBuffer<float> intermediateScratchBuffer_;
    std::array<float*, kMaxIntermediateChannels> scratchChannelPointers_ {};

    juce::AudioDeviceManager deviceManager_;
    std::atomic<AudioBackendState> state_{AudioBackendState::Uninitialized};
    std::atomic<bool> isBypassed_{false};

    mutable std::mutex errorMutex_;
    std::string lastError_;

    std::mutex callbackMutex_;
    AudioProcessCallback processCallback_;
    AudioStateCallback stateCallback_;
};

} // namespace dash::audio
