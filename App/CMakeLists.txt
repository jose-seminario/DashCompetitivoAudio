cmake_minimum_required(VERSION 3.22)

juce_add_gui_app(DashCompetitivoAudio
    PRODUCT_NAME "Dash Competitivo Audio"
    COMPANY_NAME "DashAudio"
    BUNDLE_ID    "com.dashaudio.dashcompetitivo"
)

juce_generate_juce_header(DashCompetitivoAudio)

target_sources(DashCompetitivoAudio
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.h
        Source/MainComponent.cpp
        Source/Core/AudioPolicyManager.h
        Source/Core/AudioPolicyManager.cpp
        Source/Core/ApoConfigInjector.h
        Source/Core/ApoConfigInjector.cpp
        Source/Core/ApoConfigManager.h
        Source/Core/ApoConfigManager.cpp
        Source/Audio/AudioDeviceInfo.h
        Source/Audio/AudioBackendState.h
        Source/Audio/IAudioBackend.h
        Source/Audio/JuceWasapiAudioBackend.h
        Source/Audio/JuceWasapiAudioBackend.cpp
        Source/Core/SystemDependencyChecker.h
        Source/Core/SystemDependencyChecker.cpp
        Source/Profiles/AudioProfile.h
        Source/Profiles/AudioProfile.cpp
        Source/Profiles/ProfileValidator.h
        Source/Profiles/ProfileValidator.cpp
        Source/Profiles/ProfileSerializer.h
        Source/Profiles/ProfileSerializer.cpp
        Source/Profiles/ProfileMigrator.h
        Source/Profiles/ProfileMigrator.cpp
        Source/Profiles/ProfileRepository.h
        Source/Profiles/ProfileRepository.cpp
        Source/Theme/DashLookAndFeel.h
        Source/Theme/DashLookAndFeel.cpp
        Source/Components/StatusBarComponent.h
        Source/Components/StatusBarComponent.cpp
        Source/Components/DashboardComponent.h
        Source/Components/DashboardComponent.cpp
        Source/Components/ProfilePanel.h
        Source/Components/ProfilePanel.cpp
        Source/Components/AudioEnginePanel.h
        Source/Components/AudioEnginePanel.cpp
        Source/Components/DynamicsPanel.h
        Source/Components/DynamicsPanel.cpp
        Source/Components/TacticalProcessingPanel.h
        Source/Components/TacticalProcessingPanel.cpp
        Source/Components/EqualizerPanel.h
        Source/Components/EqualizerPanel.cpp
        Source/Components/SpatialAudioPanel.h
        Source/Components/SpatialAudioPanel.cpp
        Source/Components/HeadphonePanel.h
        Source/Components/HeadphonePanel.cpp
        Source/Components/BenchmarkPanel.h
        Source/Components/BenchmarkPanel.cpp
        Source/Components/SystemStatusPanel.h
        Source/Components/SystemStatusPanel.cpp
        Source/Components/OnboardingWizard.h
        Source/Components/OnboardingWizard.cpp
)

target_include_directories(DashCompetitivoAudio
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(DashCompetitivoAudio
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DashCompetitivoAudio,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DashCompetitivoAudio,JUCE_VERSION>"
)

target_link_libraries(DashCompetitivoAudio
    PRIVATE
        DashAudioCore
        DashAudioSpatial
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_core
        juce::juce_events
        $<$<PLATFORM_ID:Windows>:ole32 propsys uuid shell32 advapi32 urlmon>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
