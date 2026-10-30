cmake_minimum_required(VERSION 3.22)
project(SmartEQ VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SmartEQ
    COMPANY_NAME "SmartEQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE SmEq
    PLUGIN_CODE Smeq
    FORMATS VST3 Standalone
    PRODUCT_NAME "Smart EQ"
)

juce_generate_juce_header(SmartEQ)

target_sources(SmartEQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SmartAnalyzer.cpp
    Source/EQGraph.cpp
)

target_compile_definitions(SmartEQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SmartEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)