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
#include <DashAudioCore/AudioConstants.h>
#include <DashAudioCore/AudioGraph.h>
#include <DashAudioCore/ParametricEQProcessor.h>

namespace dash::ui
{

/**
 * Componente Principal del Ecualizador Táctico Estéreo de 6 Bandas (MainComponent).
 *
 * Incluye:
 *  - Detección de Equalizer APO con pantalla de bloqueo / tutorial si no está instalado.
 *  - Interfaz de 2 pestañas ("ECUALIZADOR" con faders de ±18 dB y "AJUSTES").
 *  - Botón de auto-configuración directa de Equalizer APO en la pestaña de Ajustes.
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
    class ApoOnboardingOverlay;

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
    bool executeApoSync(std::string* outError = nullptr);
    void checkApoInstallation(bool userInitiated = false);

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
    std::unique_ptr<ApoOnboardingOverlay>  apoOverlay_;
    juce::TabbedComponent tabs_ { juce::TabbedButtonBar::TabsAtTop };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace dash::ui
