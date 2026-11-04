cmake_minimum_required(VERSION 3.22)

juce_add_console_app(DashAudioTests
    PRODUCT_NAME "DashAudioTests"
)

target_sources(DashAudioTests
    PRIVATE
        TestRunnerMain.cpp

        # 1. DashAudioCore Unit Tests (Estéreo 6 Bandas + Limitador)
        DashAudioCore/EQProcessorTests.cpp
        DashAudioCore/LimiterProcessorTests.cpp
        DashAudioCore/AudioGraphTests.cpp

        # 2. Windows Equalizer APO Integration Unit Tests
        App/ApoConfigManagerTests.cpp

        # Unidades desacopladas de App bajo prueba (sin dependencias GUI)
        "${CMAKE_SOURCE_DIR}/App/Source/Core/AppSettingsStore.cpp"
        "${CMAKE_SOURCE_DIR}/App/Source/Core/ApoConfigInjector.cpp"
        "${CMAKE_SOURCE_DIR}/App/Source/Core/ApoConfigManager.cpp"
        "${CMAKE_SOURCE_DIR}/App/Source/Core/AudioPolicyManager.cpp"
)

target_include_directories(DashAudioTests
    PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/App/Source"
)

target_compile_definitions(DashAudioTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Mantener activas las aserciones (<cassert>) incluso al ejecutar CTest en modo Release
if(MSVC)
    target_compile_options(DashAudioTests PRIVATE /UNDEBUG)
else()
    target_compile_options(DashAudioTests PRIVATE -UNDEBUG)
endif()

target_link_libraries(DashAudioTests
    PRIVATE
        DashAudioCore
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
        $<$<PLATFORM_ID:Windows>:advapi32 ole32 shell32 propsys>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

set(DASH_TEST_SUITES
    EQProcessorTests
    LimiterProcessorTests
    AudioGraphTests
    ApoConfigManagerTests
)

foreach(SUITE_NAME IN LISTS DASH_TEST_SUITES)
    add_test(
        NAME "DashAudio.${SUITE_NAME}"
        COMMAND DashAudioTests "${SUITE_NAME}"
    )
endforeach()
