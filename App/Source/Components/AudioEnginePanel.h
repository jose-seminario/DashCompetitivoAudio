#pragma once

#include <JuceHeader.h>
#include "../Core/AudioPolicyManager.h"

namespace dash::ui
{

/**
 * Panel de Configuración del Motor de Audio y Enrutamiento Windows (AudioEnginePanel).
 *
 * Administra los dispositivos de reproducción WASAPI mediante AudioPolicyManager,
 * la frecuencia de muestreo, el tamaño de búfer y el layout de canales de entrada.
 */
class AudioEnginePanel final : public juce::Component
{
public:
    AudioEnginePanel();
    ~AudioEnginePanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshDevices();

    std::function<void(const std::wstring& endpointId)> onEndpointChanged;
    std::function<void(double sampleRate, int bufferSize, int numChannels)> onFormatConfigChanged;

private:
    dash::audio::AudioPolicyManager policyManager_;
    std::vector<dash::audio::AudioEndpointInfo> endpoints_;

    juce::Label titleLabel_;
    juce::Label deviceSelectLabel_;
    juce::ComboBox deviceSelector_;
    juce::TextButton refreshDevicesBtn_;
    juce::TextButton setAsDefaultBtn_;

    juce::Label sampleRateLabel_;
    juce::ComboBox sampleRateSelector_;

    juce::Label bufferSizeLabel_;
    juce::ComboBox bufferSizeSelector_;

    juce::Label layoutLabel_;
    juce::ComboBox layoutSelector_;

    juce::Label infoTextLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEnginePanel)
};

} // namespace dash::ui
