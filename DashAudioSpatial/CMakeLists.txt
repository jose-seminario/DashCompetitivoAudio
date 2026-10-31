cmake_minimum_required(VERSION 3.22)

add_library(DashAudioSpatial STATIC
    ChannelLayout.h
    ChannelLayout.cpp
    ChannelMapping.h
    ChannelMapping.cpp
    HRTFProfile.h
    HRTFProfile.cpp
    HRIRDataset.h
    HRIRDataset.cpp
    ConvolutionEngine.h
    ConvolutionEngine.cpp
    BinauralRenderer.h
    BinauralRenderer.cpp
    SpatialProcessor.h
    SpatialProcessor.cpp
    HeSuViAdapter.h
    HeSuViAdapter.cpp
)

target_include_directories(DashAudioSpatial
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
    PRIVATE
        $<TARGET_PROPERTY:juce::juce_core,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:juce::juce_audio_basics,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:juce::juce_audio_formats,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:juce::juce_dsp,INTERFACE_INCLUDE_DIRECTORIES>
)

if(JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/modules")
    target_include_directories(DashAudioSpatial PRIVATE "${JUCE_SOURCE_DIR}/modules")
elseif(juce_SOURCE_DIR AND EXISTS "${juce_SOURCE_DIR}/modules")
    target_include_directories(DashAudioSpatial PRIVATE "${juce_SOURCE_DIR}/modules")
endif()

target_compile_definitions(DashAudioSpatial
    PRIVATE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_MODULE_AVAILABLE_juce_core=1
        JUCE_MODULE_AVAILABLE_juce_audio_basics=1
        JUCE_MODULE_AVAILABLE_juce_audio_formats=1
        JUCE_MODULE_AVAILABLE_juce_dsp=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Propagamos los módulos JUCE como INTERFACE hacia los binarios finales (App y Tests)
# para evitar compilar unidades de traducción de JUCE por duplicado (error MSVC LNK2005).
target_link_libraries(DashAudioSpatial
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    INTERFACE
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_dsp
)
