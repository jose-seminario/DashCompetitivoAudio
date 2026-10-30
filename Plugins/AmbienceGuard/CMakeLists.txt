cmake_minimum_required(VERSION 3.22)

juce_add_plugin(AmbienceGuard
    COMPANY_NAME "DashAudio"
    BUNDLE_ID "com.dashaudio.ambienceguard"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE DshA
    PLUGIN_CODE Dsag
    FORMATS VST3 Standalone
    PRODUCT_NAME "AmbienceGuard"
)

juce_generate_juce_header(AmbienceGuard)

target_sources(AmbienceGuard
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
)

target_include_directories(AmbienceGuard
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(AmbienceGuard
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(AmbienceGuard
    PRIVATE
        DashAudioCore
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
