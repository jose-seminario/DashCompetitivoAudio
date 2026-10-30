#pragma once

#include <JuceHeader.h>

namespace dash::ui
{

/**
 * Barra de Estado Inferior en Tiempo Real (StatusBarComponent).
 *
 * Muestra el estado del motor de audio, el perfil activo, la frecuencia de muestreo,
 * el indicador de clipping y mensajes de diagnóstico del sistema.
 */
class StatusBarComponent final : public juce::Component
{
public:
    StatusBarComponent();
    ~StatusBarComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setEngineStatus(const juce::String& statusText, bool isRunning);
    void setActiveProfileName(const juce::String& profileName);
    void setAudioFormatInfo(double sampleRate, const juce::String& layoutName);
    void setClippingDetected(bool hasClipped);
    void setStatusMessage(const juce::String& message, bool isError = false);

private:
    juce::Label engineStatusLabel_;
    juce::Label activeProfileLabel_;
    juce::Label formatInfoLabel_;
    juce::Label statusMessageLabel_;

    bool isRunning_ = false;
    bool hasClipped_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBarComponent)
};

} // namespace dash::ui
