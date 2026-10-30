#pragma once

#include <JuceHeader.h>
#include <filesystem>
#include <memory>
#include <string>

#include "Theme/DashLookAndFeel.h"
#include "Components/StatusBarComponent.h"
#include "Components/DashboardComponent.h"
#include "Components/ProfilePanel.h"
#include "Components/AudioEnginePanel.h"
#include "Components/DynamicsPanel.h"
#include "Components/TacticalProcessingPanel.h"
#include "Components/EqualizerPanel.h"
#include "Components/SpatialAudioPanel.h"
#include "Components/HeadphonePanel.h"
#include "Components/BenchmarkPanel.h"
#include "Components/SystemStatusPanel.h"

#include "Core/AudioPolicyManager.h"
#include "Core/ApoConfigInjector.h"
#include "Core/ApoConfigManager.h"
#include "Profiles/AudioProfile.h"
#include "Profiles/ProfileRepository.h"

namespace dash::ui
{

/**
 * Componente Principal y Orquestador de la Aplicación (MainComponent).
 *
 * Responsabilidad estricta:
 *  - Coordinación de vistas y navegación por paneles modulares.
 *  - Delegación de DSP a AudioGraph.
 *  - Delegación de perfiles a ProfileRepository.
 *  - Delegación de inyección de configuración Windows a ApoConfigManager.
 *  - Gestión del LookAndFeel táctico y temporizador de telemetría a 30 FPS.
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
    void timerCallback() override;

    void selectPanel(int panelIndex);
    void loadProfile(const std::string& profileId);
    void syncAllPanelsWithProfile();
    void scheduleDebouncedInjection();
    void executeConfigInjection();

    void activateTacticalMode();
    void deactivateAndRestoreAudio();

    // Estilo visual del sistema
    DashLookAndFeel dashLookAndFeel_;

    // Servicios desacoplados
    dash::audio::AudioPolicyManager policyManager_;
    dash::audio::ApoConfigManager   apoManager_;
    dash::profile::ProfileRepository profileRepo_;
    dash::profile::AudioProfile     currentProfile_;

    std::wstring originalEndpointId_;
    bool isProfileActive_ = false;
    bool needsInjection_  = false;
    int debounceCounter_  = 0;

    // Componentes de interfaz de usuario
    juce::Label headerTitleLabel_;
    juce::Label headerSubtitleLabel_;

    juce::TabbedComponent tabs_;
    StatusBarComponent   statusBar_;

    // Paneles modulares
    std::unique_ptr<DashboardComponent>      dashboardPanel_;
    std::unique_ptr<ProfilePanel>            profilePanel_;
    std::unique_ptr<AudioEnginePanel>        audioEnginePanel_;
    std::unique_ptr<DynamicsPanel>           dynamicsPanel_;
    std::unique_ptr<TacticalProcessingPanel> tacticalPanel_;
    std::unique_ptr<EqualizerPanel>          equalizerPanel_;
    std::unique_ptr<SpatialAudioPanel>       spatialPanel_;
    std::unique_ptr<HeadphonePanel>          headphonePanel_;
    std::unique_ptr<BenchmarkPanel>          benchmarkPanel_;
    std::unique_ptr<SystemStatusPanel>       systemStatusPanel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace dash::ui
