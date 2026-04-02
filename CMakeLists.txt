cmake_minimum_required(VERSION 3.22)
project(LocBox VERSION 0.3.1)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(/Users/chris/src/github/JUCE juce)

juce_add_plugin(LocBox
    COMPANY_NAME             "CorvidAudio"
    PLUGIN_MANUFACTURER_CODE CVDA
    PLUGIN_CODE              LBOX
    FORMATS                  AU VST3 Standalone
    PRODUCT_NAME             "Loc-Box"
    DESCRIPTION              "Shure Level Loc brickwall limiter emulation"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    AU_MAIN_TYPE             kAudioUnitType_Effect
    AU_EXPORT_PREFIX         LocBoxAU
    BUNDLE_IDENTIFIER        com.CorvidAudio.LocBox
    COPY_PLUGIN_AFTER_BUILD  FALSE
)

target_sources(LocBox PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_compile_definitions(LocBox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(LocBox PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_gui_basics
    juce::juce_core
    juce::juce_dsp
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce_recommended_config_flags
    juce_recommended_lto_flags
    juce_recommended_warning_flags
)

juce_generate_juce_header(LocBox)
