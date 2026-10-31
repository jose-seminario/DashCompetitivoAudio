#include "JuceWasapiAudioBackend.h"
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
        const auto names = type->getDeviceNames(false); // false = salida (render)
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
            info.isVirtualCable = names[i].containsIgnoreCase("cable") || names[i].containsIgnoreCase("virtual");
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
        const auto names = type->getDeviceNames(true); // true = entrada (capture)
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
            info.isVirtualCable = names[i].containsIgnoreCase("cable") || names[i].containsIgnoreCase("virtual");
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

    if (!config.deviceId.empty())
    {
        setup.outputDeviceName = juce::String(config.deviceId.c_str());
    }

    if (!config.inputDeviceId.empty())
    {
        setup.inputDeviceName = juce::String(config.inputDeviceId.c_str());
    }
    else
    {
        // Si no se especificó un dispositivo de entrada, buscamos un cable virtual disponible
        const auto captureDevs = enumerateCaptureDevices();
        for (const auto& dev : captureDevs)
        {
            if (dev.isVirtualCable || dev.friendlyName.find(L"CABLE") != std::wstring::npos)
            {
                setup.inputDeviceName = juce::String(dev.friendlyName.c_str());
                break;
            }
        }
    }

    setup.sampleRate = config.sampleRate > 0.0 ? config.sampleRate : 48000.0;
    setup.bufferSize = config.bufferSize > 0 ? config.bufferSize : 512;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = true;

    const int numInChannels = config.numInputChannels > 0 ? config.numInputChannels : 2;
    const int numOutChannels = config.numChannels > 0 ? config.numChannels : 2;

    auto err = deviceManager_.initialise(
        numInChannels,
        numOutChannels,
        nullptr,
        true,
        setup.outputDeviceName,
        &setup);

    // Fallback gracioso: si el dispositivo de entrada no está disponible (ej. sin cable virtual instalado),
    // inicializar sólo dispositivo de salida para garantizar continuidad.
    if (err.isNotEmpty() && numInChannels > 0)
    {
        setup.inputDeviceName.clear();
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

    deviceManager_.addAudioCallback(this);
    state_.store(AudioBackendState::Running);

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (stateCallback_)
            stateCallback_(AudioBackendState::Running, "WASAPI audio stream iniciado correctamente.");
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
    // Limpiar buffers de salida o transferir señal de entrada disponible
    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (ch < numInputChannels && inputChannelData != nullptr && inputChannelData[ch] != nullptr)
        {
            std::memcpy(outputChannelData[ch], inputChannelData[ch], static_cast<size_t>(numSamples) * sizeof(float));
        }
        else
        {
            std::memset(outputChannelData[ch], 0, static_cast<size_t>(numSamples) * sizeof(float));
        }
    }

    if (isBypassed_.load(std::memory_order_relaxed))
        return;

    // Procesamiento estrictamente lock-free en tiempo real (cero llamadas a mutex en el hilo de audio)
    if (processCallback_)
    {
        processCallback_(outputChannelData, numOutputChannels, numSamples);
    }
}

void JuceWasapiAudioBackend::audioDeviceAboutToStart(juce::AudioIODevice* /*device*/)
{
}

void JuceWasapiAudioBackend::audioDeviceStopped()
{
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
