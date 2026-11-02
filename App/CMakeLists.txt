cmake_minimum_required(VERSION 3.22)

# Forzar codificación UTF-8 en MSVC antes de cualquier target
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Crear el archivo icon.ico desde Base64 si no existe
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico")
    set(ICON_BASE64 "AAABAAEAICAAAAEAIAA1BAAAFgAAAIlQTkcNChoKAAAADUlIRFIAAAAgAAAAIAgGAAAAc3p69AAAAAFzUkdCAK7OHOkAAAAEZ0FNQQAAsY8L/GEFAAAACXBIWXMAAA7DAAAOwwHHb6hkAAADyklEQVRYR9VXX0hTYRTfg/u+6zbFXiwIQQgie8mHgsAHfSlC+iNRoD0MIYkahBBkPhSU9FBmQYP+aP8QXbJNnZM5E1LLbE4zVzoQelB6kKKSucKs9nDjfNudu+f7du/MeujAD+72fff8fufcc853r8Hwv1mW/+1uY9/MReIcHaPOQCgV8D/1hffhe9ZvnlBelivQQjyTUdrcFzEdPytbSg/IlpK9KsD/9EZHhPS+iVH3mJv2zxZiV2s2oytwhbiCEXOVLZZbWCTnbtioj/wC2XzIKhPH8wgINwzPS9ivvnlCeaTjxXj2ifqVnM1beJJMAEKqbDHaMfrO5AtvwhRpDVJH24dmLaX7f3JOU2DZVZZMP15T7SsukYnj2QejL1yMuXjzhPIYeXEJ5whgOlIjS3d6ZOIPc6AP+mWTtZZFju+Dx0fbBj/p1gWkXRQ5RCvZ3RypCCAEihT7yNm+EzIxB0FiXmZG95g9u/rMCr4RyIl3iiPSg2XPYU6EubzyF3k88gRzG6BIiDMQ5dIHqWsd5JxnAtoZZFFjEfTRwBJXD9AurNXQZqmhmXO8FkC9YJ+QGeJ8ObTKPjwvke5XX7l2KyxSOYOIpCaHTFufckRa66L5QTqDy8nWhPEJEwxvYkpTHCmdAUKBDAtIty6qhey6puWsvumTTICxa+K2+ZiN2wQtpziR7C61g3PXVeRa6zCisW/VY4CDRdQ25vLKVYKGu2qC05fUAjTWYSxj36wlO4MLcQHuwJxo8LBNCol3iqVWWcPPWWtd1Amwl3hD3+MCPJNRUaEAUqcepNlUU8eiTSXWXO8Y4XwqgPW4gM7ggkglQG8IpdaJCKICZMgvkOHYjneBMxDSOlCgQLFjhkR09H4/v+YPszrAvhTkbN0hk66Jz4kMjLdCJHhTKqSmdo4AiFnK7S5+7aZLfCglAEUPxR9vQ1+4Wrpg/4Y3pQJaCZNoQdR6yF/M6Jm8zASwI7jn9Q+8KZ0Ac4VV91pPAHcewFBIWzBIAPzWu9YSwF5Q3IG5JDkYqAFVePO/EECvti6RvukKlQAwrSz8LQHC6BWD04m2Db2HFuFuTLx2A2CK6V2L2podUg8HPpLe6W2YO2nsUdzzf9FqoT8FbWxbJN3jRzEnZ8Q5dpCJSDOe1woWeWPbInWM2DBXWoNMwKu0KJVrATxzlvZMIsfGasIZCEm3uiKi01ILUEdSQ0sUCk7zmWditH+mjDlyDEdNp87HWFYENcIK1Vorw/cjzHmjf6YS+1qXQSTEN10PWYGTTGk5BewL2Ru6Bl/Q+N509htt/HI+XAZ6ZgAAAABJRU5ErkJggg==")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico.b64" "${ICON_BASE64}")
    execute_process(COMMAND powershell -Command "[IO.File]::WriteAllBytes('${CMAKE_CURRENT_SOURCE_DIR}/icon.ico', [Convert]::FromBase64String([IO.File]::ReadAllText('${CMAKE_CURRENT_SOURCE_DIR}/icon.ico.b64')))")
    file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico.b64")
endif()

juce_add_gui_app(DashCompetitivoAudio
    PRODUCT_NAME "Dash Competitivo Audio"
    COMPANY_NAME "DashAudio"
    BUNDLE_ID    "com.dashaudio.dashcompetitivo"
    ICON_BIG     "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
    ICON_SMALL   "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
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

if(MSVC)
    target_compile_options(DashCompetitivoAudio PRIVATE /utf-8)
endif()

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
