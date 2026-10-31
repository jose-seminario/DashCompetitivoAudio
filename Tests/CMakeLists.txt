cmake_minimum_required(VERSION 3.22)

juce_add_console_app(DashAudioTests
    PRODUCT_NAME "DashAudioTests"
)

target_sources(DashAudioTests
    PRIVATE
        TestRunnerMain.cpp

        # 1. DashAudioCore Unit Tests
        DashAudioCore/DynamicsProcessorTests.cpp
        DashAudioCore/TransientProcessorTests.cpp
        DashAudioCore/AmbienceProcessorTests.cpp
        DashAudioCore/EQProcessorTests.cpp
        DashAudioCore/LimiterProcessorTests.cpp
        DashAudioCore/AudioGraphTests.cpp

        # 2. DashAudioSpatial Unit Tests
        DashAudioSpatial/ChannelLayoutTests.cpp
        DashAudioSpatial/BinauralRendererTests.cpp
        DashAudioSpatial/ConvolutionTests.cpp

        # 3. Profiles & Security Unit Tests
        Profiles/ProfileValidatorTests.cpp
        Profiles/ProfileSerializerTests.cpp
        Profiles/ProfileMigratorTests.cpp

        # 4. Windows APO Integration Unit Tests
        App/ApoConfigManagerTests.cpp

        # Unidades desacopladas de App bajo prueba (sin dependencias GUI)
        "${CMAKE_SOURCE_DIR}/App/Source/Profiles/AudioProfile.cpp"
        "${CMAKE_SOURCE_DIR}/App/Source/Profiles/ProfileValidator.cpp"
        "${CMAKE_SOURCE_DIR}/App/Source/Profiles/ProfileSerializer.cpp"
        "${CMAKE_SOURCE_DIR}/App/Source/Profiles/ProfileMigrator.cpp"
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
        DashAudioSpatial
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_dsp
        $<$<PLATFORM_ID:Windows>:advapi32 ole32 shell32 propsys>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Registro individual de las 13 suites en CTest
set(DASH_TEST_SUITES
    DynamicsProcessorTests
    TransientProcessorTests
    AmbienceProcessorTests
    EQProcessorTests
    LimiterProcessorTests
    AudioGraphTests
    ChannelLayoutTests
    BinauralRendererTests
    ConvolutionTests
    ProfileValidatorTests
    ProfileSerializerTests
    ProfileMigratorTests
    ApoConfigManagerTests
)

foreach(SUITE_NAME IN LISTS DASH_TEST_SUITES)
    add_test(
        NAME "DashAudio.${SUITE_NAME}"
        COMMAND DashAudioTests "${SUITE_NAME}"
    )
endforeach()
