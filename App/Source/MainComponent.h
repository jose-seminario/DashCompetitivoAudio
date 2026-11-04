#pragma once

#include <JuceHeader.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "Theme/DashLookAndFeel.h"
#include "Core/AudioPolicyManager.h"
#include "Core/ApoConfigInjector.h"
#include "Core/ApoConfigManager.h"
#include "Audio/JuceWasapiAudioBackend.h"
#include <DashAudioCore/AudioGraph.h>
#include <DashAudioCore/ParametricEQProcessor.h>

namespace dash::ui
{

/**
 * Componente Principal del Ecualizador Táctico Estéreo de 6 Bandas (MainComponent).
 *
 * Interfaz minimalista compuesta estrictamente por 2 pestañas:
 *  1. "ECUALIZADOR": Fader vertical de Preamp + Reset a la izquierda y 6 faders verticales tácticos a la derecha.
 *  2. "AJUSTES": Selector de dispositivo de salida estéreo, Bypass global y telemetría básica (CPU / Latencia).
 */
class MainComponent final
    : public juce::Component,
      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class EqualizerTabComponent;
    class SettingsTabComponent;

    struct OutputEndpointEntry
    {
        std::wstring id;
        std::wstring friendlyName;
        std::wstring endpointGuid;
    };

    void timerCallback() override;

    void populateOutputDevices();
    void restartAudioStream();
    void syncDspParameters();
    void resetAllEqAndPreamp();
    void scheduleDebouncedApoSync();
    void executeApoSync();

    DashLookAndFeel dashLookAndFeel_;

    dash::audio::AudioPolicyManager                      policyManager_;
    dash::core::AudioGraphParameters                     graphParams_;
    std::unique_ptr<dash::core::AudioGraph>              audioGraph_;
    std::unique_ptr<dash::audio::JuceWasapiAudioBackend> audioBackend_;

    std::vector<OutputEndpointEntry> outputEndpoints_;
    std::wstring selectedDeviceName_;
    std::string  selectedEndpointGuid_;

    bool needsApoSync_    = false;
    int  debounceCounter_ = 0;

    std::unique_ptr<EqualizerTabComponent> equalizerTab_;
    std::unique_ptr<SettingsTabComponent>  settingsTab_;
    juce::TabbedComponent tabs_ { juce::TabbedButtonBar::TabsAtTop };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace dash::ui
