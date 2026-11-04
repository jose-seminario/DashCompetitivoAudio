cmake_minimum_required(VERSION 3.22)

add_library(DashAudioCore STATIC
    AudioConstants.h
    AudioGraphParameters.h
    AudioGraph.h
    AudioGraph.cpp
    LimiterProcessor.h
    LimiterProcessor.cpp
    ParametricEQProcessor.h
    ParametricEQProcessor.cpp
)

target_include_directories(DashAudioCore
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
    PRIVATE
        $<TARGET_PROPERTY:juce::juce_dsp,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:juce::juce_core,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:juce::juce_audio_basics,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:juce::juce_audio_formats,INTERFACE_INCLUDE_DIRECTORIES>
)

if(JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/modules")
    target_include_directories(DashAudioCore PRIVATE "${JUCE_SOURCE_DIR}/modules")
elseif(juce_SOURCE_DIR AND EXISTS "${juce_SOURCE_DIR}/modules")
    target_include_directories(DashAudioCore PRIVATE "${juce_SOURCE_DIR}/modules")
endif()

target_compile_definitions(DashAudioCore
    PRIVATE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_MODULE_AVAILABLE_juce_core=1
        JUCE_MODULE_AVAILABLE_juce_audio_basics=1
        JUCE_MODULE_AVAILABLE_juce_audio_formats=1
        JUCE_MODULE_AVAILABLE_juce_dsp=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(DashAudioCore
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    INTERFACE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
)
