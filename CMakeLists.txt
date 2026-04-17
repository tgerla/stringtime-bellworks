cmake_minimum_required(VERSION 3.22)

project(StringtimeBellworks VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Pull JUCE at configure time.
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(StringtimeBellworks
    COMPANY_NAME "Local"
    BUNDLE_ID com.local.stringtimebellworks
    PLUGIN_MANUFACTURER_CODE Locl
    PLUGIN_CODE Mdl2
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Stringtime Bellworks"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(StringtimeBellworks)

target_sources(StringtimeBellworks
    PRIVATE
        Source/ModalVoice.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
)

target_include_directories(StringtimeBellworks
    PRIVATE
        Source
)

target_compile_definitions(StringtimeBellworks
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(StringtimeBellworks
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
