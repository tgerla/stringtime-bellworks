cmake_minimum_required(VERSION 3.22)

project(StringtimeBellworks VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

set(STRINGTIME_PLUGIN_NAME "Stringtime Bellworks")

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
    PRODUCT_NAME "${STRINGTIME_PLUGIN_NAME}"
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

    if(APPLE)
        set(PACKAGE_STAGING_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-$<CONFIG>-macOS")

        add_custom_target(package_macos
        DEPENDS
            StringtimeBellworks_AU
            StringtimeBellworks_VST3
            StringtimeBellworks_Standalone
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/${PACKAGE_STAGING_NAME}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${PACKAGE_STAGING_NAME}/AU"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${PACKAGE_STAGING_NAME}/VST3"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${PACKAGE_STAGING_NAME}/Standalone"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_artefacts/$<CONFIG>/AU/${STRINGTIME_PLUGIN_NAME}.component"
            "${CMAKE_BINARY_DIR}/${PACKAGE_STAGING_NAME}/AU/${STRINGTIME_PLUGIN_NAME}.component"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_artefacts/$<CONFIG>/VST3/${STRINGTIME_PLUGIN_NAME}.vst3"
            "${CMAKE_BINARY_DIR}/${PACKAGE_STAGING_NAME}/VST3/${STRINGTIME_PLUGIN_NAME}.vst3"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_artefacts/$<CONFIG>/Standalone/${STRINGTIME_PLUGIN_NAME}.app"
            "${CMAKE_BINARY_DIR}/${PACKAGE_STAGING_NAME}/Standalone/${STRINGTIME_PLUGIN_NAME}.app"
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_BINARY_DIR}/dist/${PACKAGE_STAGING_NAME}.zip" --format=zip "${PACKAGE_STAGING_NAME}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Staging and zipping AU/VST3/Standalone artifacts for macOS redistribution"
        VERBATIM
        )
    endif()
