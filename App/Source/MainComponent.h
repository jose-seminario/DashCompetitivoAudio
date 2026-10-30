#pragma once

#include <JuceHeader.h>
#include <vector>

#include "Core/AudioPolicyManager.h"
#include "Core/ApoConfigInjector.h"

namespace dash::ui
{

class MainComponent final : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void refreshEndpoints();
    void applySelectedEndpoint();
    void syncConfigFromControls();
    void updateConfigPreview();
    void injectApoConfig();
    void logStatus(const juce::String& message, bool isError = false);

    dash::audio::AudioPolicyManager policyManager_;
    std::vector<dash::audio::AudioEndpointInfo> endpoints_;
    dash::audio::ApoChainConfig apoConfig_;

    // Encabezado
    juce::Label titleLabel_;
    juce::Label subtitleLabel_;

    // Sección 1: Orquestador COM de Dispositivos de Windows (IPolicyConfig)
    juce::GroupComponent endpointGroup_ { "endpointGroup", "1. Orquestador COM de Salida WASAPI (IPolicyConfig)" };
    juce::ComboBox endpointCombo_;
    juce::TextButton refreshEndpointsBtn_ { "Actualizar" };
    juce::TextButton setDefaultEndpointBtn_ { "Establecer Predeterminado (Consola + Multimedia)" };

    // Sección 2: Cadena DSP e Inyector Equalizer APO
    juce::GroupComponent apoGroup_ { "apoGroup", "2. Cadena Táctica DSP e Inyector Atómico Equalizer APO" };
    juce::ToggleButton bindSelectedDeviceToggle_ { "Filtrar cadena por GUID del endpoint seleccionado" };
    juce::ToggleButton transientToggle_ { "Supresor de Transitorios (TransientSuppressor.dll)" };
    juce::ToggleButton compressorToggle_ { "Compresor Híbrido BS.1770 (LoudnessCompressor.dll)" };
    juce::ToggleButton ambienceToggle_ { "Protector Ambiental Multibanda (AmbienceGuard.dll)" };
    juce::ToggleButton graphicEqToggle_ { "Ecualizador Táctico 31 Bandas + Preamp -6 dB" };
    juce::ToggleButton hesuviToggle_ { "Virtualización Binaural HRIR 7.1 (HeSuVi)" };

    juce::Label configPathLabel_ { "configPathLabel", "Ruta config.txt:" };
    juce::TextEditor configPathEditor_;
    juce::TextButton injectConfigBtn_ { "Inyectar config.txt Atómicamente" };

    juce::Label previewLabel_ { "previewLabel", "Vista previa en tiempo real de directivas Equalizer APO:" };
    juce::TextEditor configPreviewEditor_;

    // Barra de estado / telemetría de operaciones
    juce::Label statusLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace dash::ui
