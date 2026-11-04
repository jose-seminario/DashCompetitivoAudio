#include "JuceWasapiAudioBackend.h"

#include <algorithm>
#include <cstring>

namespace dash::audio
{

JuceWasapiAudioBackend::JuceWasapiAudioBackend() = default;

JuceWasapiAudioBackend::~JuceWasapiAudioBackend()
{
    shutdown();
}

bool JuceWasapiAudioBackend::initialize(std::string* /*outError*/)
{
    const auto& types = deviceManager_.getAvailableDeviceTypes();
    bool foundWasapi = false;
    for (auto* type : types)
    {
        if (type->getTypeName() == "Windows Audio" || type->getTypeName().containsIgnoreCase("WASAPI"))
        {
            deviceManager_.setCurrentAudioDeviceType(type->getTypeName(), true);
            foundWasapi = true;
            break;
        }
    }
    if (!foundWasapi && !types.isEmpty())
    {
        deviceManager_.setCurrentAudioDeviceType(types[0]->getTypeName(), true);
    }

    state_.store(AudioBackendState::Stopped);
    return true;
}

void JuceWasapiAudioBackend::shutdown()
{
    stop();
    deviceManager_.closeAudioDevice();
    state_.store(AudioBackendState::Uninitialized);
}

std::vector<AudioDeviceInfo> JuceWasapiAudioBackend::enumerateRenderDevices(std::string* /*outError*/)
{
    std::vector<AudioDeviceInfo> result;
    if (auto* type = deviceManager_.getCurrentDeviceTypeObject())
    {
        type->scanForDevices();
        const auto names = type->getDeviceNames(false);
        const int defaultIdx = type->getDefaultDeviceIndex(false);

        for (int i = 0; i < names.size(); ++i)
        {
            AudioDeviceInfo info;
            info.friendlyName = names[i].toWideCharPointer();
            info.id = names[i].toWideCharPointer();
            info.endpointGuid = names[i].toWideCharPointer();
            info.preferredSampleRate = 48000;
            info.preferredChannels = 2;
            info.isDefault = (i == defaultIdx);
            result.push_back(std::move(info));
        }
    }
    return result;
}

std::vector<AudioDeviceInfo> JuceWasapiAudioBackend::enumerateCaptureDevices(std::string* /*outError*/)
{
    std::vector<AudioDeviceInfo> result;
    if (auto* type = deviceManager_.getCurrentDeviceTypeObject())
    {
        type->scanForDevices();
        const auto names = type->getDeviceNames(true);
        const int defaultIdx = type->getDefaultDeviceIndex(true);

        for (int i = 0; i < names.size(); ++i)
        {
            AudioDeviceInfo info;
            info.friendlyName = names[i].toWideCharPointer();
            info.id = names[i].toWideCharPointer();
            info.endpointGuid = names[i].toWideCharPointer();
            info.preferredSampleRate = 48000;
            info.preferredChannels = 2;
            info.isDefault = (i == defaultIdx);
            result.push_back(std::move(info));
        }
    }
    return result;
}

bool JuceWasapiAudioBackend::start(
    const AudioBackendConfig& config,
    AudioProcessCallback processCallback,
    AudioStateCallback stateCallback,
    std::string* outError)
{
    stop();

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        processCallback_ = std::move(processCallback);
        stateCallback_ = std::move(stateCallback);
    }

    if (config.exclusiveMode)
    {
        deviceManager_.setCurrentAudioDeviceType("Windows Audio (Exclusive)", true);
    }
    else
    {
        deviceManager_.setCurrentAudioDeviceType("Windows Audio", true);
    }

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager_.getAudioDeviceSetup(setup);

    const auto renderDevs = enumerateRenderDevices();
    juce::String chosenOutputName;

    if (!config.deviceId.empty())
    {
        juce::String target(config.deviceId.c_str());
        for (const auto& dev : renderDevs)
        {
            juce::String dName = juce::String(dev.friendlyName.c_str());
            if (dName.equalsIgnoreCase(target) || dName.containsIgnoreCase(target) || target.containsIgnoreCase(dName))
            {
                chosenOutputName = dName;
                break;
            }
        }
    }

    if (chosenOutputName.isEmpty())
    {
        for (const auto& dev : renderDevs)
        {
            if (dev.isDefault)
            {
                chosenOutputName = juce::String(dev.friendlyName.c_str());
                break;
            }
        }
    }

    if (chosenOutputName.isEmpty() && !renderDevs.empty())
    {
        chosenOutputName = juce::String(renderDevs[0].friendlyName.c_str());
    }

    if (chosenOutputName.isNotEmpty())
    {
        setup.outputDeviceName = chosenOutputName;
    }

    const auto captureDevs = enumerateCaptureDevices();
    juce::String chosenInputName;

    if (!config.inputDeviceId.empty())
    {
        juce::String target(config.inputDeviceId.c_str());
        for (const auto& dev : captureDevs)
        {
            juce::String dName = juce::String(dev.friendlyName.c_str());
            if (dName.equalsIgnoreCase(target) || dName.containsIgnoreCase(target) || target.containsIgnoreCase(dName))
            {
                chosenInputName = dName;
                break;
            }
        }
    }

    if (chosenInputName.isNotEmpty())
    {
        setup.inputDeviceName = chosenInputName;
    }

    setup.sampleRate = config.sampleRate > 0.0 ? config.sampleRate : 48000.0;
    setup.bufferSize = config.bufferSize > 0 ? config.bufferSize : 512;
    const int numInChannels  = std::clamp(config.numInputChannels > 0 ? config.numInputChannels : 2, 0, kMaxIntermediateChannels);
    const int numOutChannels = std::clamp(config.numChannels > 0 ? config.numChannels : 2, 1, kMaxIntermediateChannels);

    setup.useDefaultInputChannels = (numInChannels > 0);
    setup.useDefaultOutputChannels = true;
    setup.inputChannels.clear();
    setup.outputChannels.clear();
    if (numInChannels > 0)
        setup.inputChannels.setRange(0, numInChannels, true);
    setup.outputChannels.setRange(0, numOutChannels, true);

    auto err = deviceManager_.initialise(
        numInChannels,
        numOutChannels,
        nullptr,
        true,
        setup.outputDeviceName,
        &setup);

    // Fallback a solo salida estéreo si no hay dispositivo de captura disponible
    if (err.isNotEmpty() && numInChannels > 0)
    {
        setup.inputDeviceName.clear();
        setup.inputChannels.clear();
        err = deviceManager_.initialise(
            0,
            numOutChannels,
            nullptr,
            true,
            setup.outputDeviceName,
            &setup);
    }

    if (err.isNotEmpty())
    {
        {
            std::lock_guard<std::mutex> lock(errorMutex_);
            lastError_ = err.toStdString();
        }
        if (outError)
            *outError = err.toStdString();

        state_.store(AudioBackendState::Error);
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (stateCallback_)
            stateCallback_(AudioBackendState::Error, err.toStdString());
        return false;
    }

    const int initialCapacity = std::max(2048, setup.bufferSize * 2);
    intermediateScratchBuffer_.setSize(kMaxIntermediateChannels, initialCapacity);
    intermediateScratchBuffer_.clear();
    for (int ch = 0; ch < kMaxIntermediateChannels; ++ch)
        scratchChannelPointers_[static_cast<size_t>(ch)] = intermediateScratchBuffer_.getWritePointer(ch);

    deviceManager_.addAudioCallback(this);
    state_.store(AudioBackendState::Running);

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (stateCallback_)
            stateCallback_(AudioBackendState::Running, "WASAPI estéreo iniciado correctamente.");
    }
    return true;
}

bool JuceWasapiAudioBackend::stop(std::string* /*outError*/)
{
    deviceManager_.removeAudioCallback(this);
    deviceManager_.closeAudioDevice();
    state_.store(AudioBackendState::Stopped);

    std::lock_guard<std::mutex> lock(callbackMutex_);
    processCallback_ = nullptr;
    stateCallback_ = nullptr;
    return true;
}

AudioBackendState JuceWasapiAudioBackend::getState() const noexcept
{
    if (state_.load() == AudioBackendState::Running && isBypassed_.load())
        return AudioBackendState::Bypassed;
    return state_.load();
}

void JuceWasapiAudioBackend::setBypass(bool bypassed) noexcept
{
    isBypassed_.store(bypassed);
}

bool JuceWasapiAudioBackend::isBypassed() const noexcept
{
    return isBypassed_.load();
}

std::string JuceWasapiAudioBackend::getLastError() const
{
    std::lock_guard<std::mutex> lock(errorMutex_);
    return lastError_;
}

float JuceWasapiAudioBackend::getCurrentLatencyMs() const noexcept
{
    if (auto* currentDevice = const_cast<juce::AudioDeviceManager&>(deviceManager_).getCurrentAudioDevice())
    {
        const double sr = currentDevice->getCurrentSampleRate();
        if (sr > 0.0)
        {
            const int totalLatencySamples = currentDevice->getInputLatencyInSamples() + currentDevice->getOutputLatencyInSamples();
            return static_cast<float>((totalLatencySamples * 1000.0) / sr);
        }
    }
    return 0.0f;
}

float JuceWasapiAudioBackend::getCpuUsage() const noexcept
{
    return static_cast<float>(const_cast<juce::AudioDeviceManager&>(deviceManager_).getCpuUsage() * 100.0);
}

void JuceWasapiAudioBackend::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& /*context*/)
{
    if (outputChannelData == nullptr || numOutputChannels <= 0 || numSamples <= 0)
        return;

    const int availableScratchSamples = intermediateScratchBuffer_.getNumSamples();
    if (numSamples > availableScratchSamples)
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            if (outputChannelData[ch] != nullptr)
                std::memset(outputChannelData[ch], 0, static_cast<size_t>(numSamples) * sizeof(float));
        }
        return;
    }

    const int activeChannels = std::clamp(std::max(numInputChannels, numOutputChannels), 1, kMaxIntermediateChannels);

    for (int ch = 0; ch < activeChannels; ++ch)
    {
        float* dst = scratchChannelPointers_[static_cast<size_t>(ch)];
        if (dst == nullptr)
            continue;

        if (ch < numInputChannels && inputChannelData != nullptr && inputChannelData[ch] != nullptr)
        {
            std::memcpy(dst, inputChannelData[ch], static_cast<size_t>(numSamples) * sizeof(float));
        }
        else
        {
            std::memset(dst, 0, static_cast<size_t>(numSamples) * sizeof(float));
        }
    }

    if (isBypassed_.load(std::memory_order_relaxed))
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            if (outputChannelData[ch] != nullptr)
            {
                if (ch < activeChannels)
                    std::memcpy(outputChannelData[ch], scratchChannelPointers_[static_cast<size_t>(ch)], static_cast<size_t>(numSamples) * sizeof(float));
                else
                    std::memset(outputChannelData[ch], 0, static_cast<size_t>(numSamples) * sizeof(float));
            }
        }
        return;
    }

    if (processCallback_)
    {
        processCallback_(scratchChannelPointers_.data(), activeChannels, numSamples);
    }

    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            if (ch < activeChannels)
                std::memcpy(outputChannelData[ch], scratchChannelPointers_[static_cast<size_t>(ch)], static_cast<size_t>(numSamples) * sizeof(float));
            else
                std::memset(outputChannelData[ch], 0, static_cast<size_t>(numSamples) * sizeof(float));
        }
    }
}

void JuceWasapiAudioBackend::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const int bufferSize = (device != nullptr) ? device->getCurrentBufferSizeSamples() : 512;
    const int safeSamples = std::max(2048, bufferSize * 2);
    intermediateScratchBuffer_.setSize(kMaxIntermediateChannels, safeSamples);
    intermediateScratchBuffer_.clear();
    for (int ch = 0; ch < kMaxIntermediateChannels; ++ch)
        scratchChannelPointers_[static_cast<size_t>(ch)] = intermediateScratchBuffer_.getWritePointer(ch);
}

void JuceWasapiAudioBackend::audioDeviceStopped()
{
    intermediateScratchBuffer_.clear();
}

void JuceWasapiAudioBackend::audioDeviceError(const juce::String& errorMessage)
{
    {
        std::lock_guard<std::mutex> lock(errorMutex_);
        lastError_ = errorMessage.toStdString();
    }
    state_.store(AudioBackendState::Error);
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (stateCallback_)
    {
        stateCallback_(AudioBackendState::Error, errorMessage.toStdString());
    }
}

} // namespace dash::audio
