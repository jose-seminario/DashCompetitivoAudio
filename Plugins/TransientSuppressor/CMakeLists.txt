cmake_minimum_required(VERSION 3.22)

juce_add_plugin(TransientSuppressor
    COMPANY_NAME "DashAudio"
    BUNDLE_ID "com.dashaudio.transientsuppressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE DshA
    PLUGIN_CODE Dsts
    FORMATS VST3 Standalone
    PRODUCT_NAME "TransientSuppressor"
)

juce_generate_juce_header(TransientSuppressor)

target_sources(TransientSuppressor
    PRIVATE
        Source/DSP/CenterTransientSuppressor.h
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
)

target_include_directories(TransientSuppressor
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(TransientSuppressor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(TransientSuppressor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
