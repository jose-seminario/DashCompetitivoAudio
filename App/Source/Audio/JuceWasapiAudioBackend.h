#pragma once

#include "IAudioBackend.h"
#include <JuceHeader.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace dash::audio
{

/**
 * Backend de audio nativo de Windows basado en WASAPI implementado sobre juce::AudioDeviceManager.
 *
 * Provee procesamiento de audio en tiempo real y comunicación de bajo retardo
 * sin requerir la presencia de Equalizer APO.
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
