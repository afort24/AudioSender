cmake_minimum_required(VERSION 3.22)
project(AudioSender VERSION 1.1.0)
set(CMAKE_CXX_STANDARD 17)

# Use CPM to get JUCE
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
include(cmake/cpm.cmake)

cpmaddpackage(
        NAME JUCE
        GIT_TAG 8.0.6
        VERSION 8.0.6
        GITHUB_REPOSITORY juce-framework/JUCE
        SOURCE_DIR ${LIB_DIR}/juce
)

# Set up your plugin
juce_add_plugin(AudioSender
        VERSION 1.1.0
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "AudioSender"
        COMPANY_NAME AlexFortunatoMusic

        IS_SYNTH FALSE                       # Is this a synth or an effect?
        NEEDS_MIDI_INPUT FALSE               # Does the plugin need midi input?
        NEEDS_MIDI_OUTPUT FALSE              # Does the plugin need midi output?
        IS_MIDI_EFFECT FALSE                 # Is this plugin a MIDI effect?
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE    # Does the editor need keyboard focus?
        COPY_PLUGIN_AFTER_BUILD TRUE         # Should the plugin be installed to a default location after building?
        PLUGIN_MANUFACTURER_CODE Fort        # A four-character manufacturer id with at least one upper-case character
        PLUGIN_CODE Sndr                     # A unique four-character plugin id with exactly one upper-case character
)

# Generate JuceHeader.h
juce_generate_juce_header(AudioSender)

# Add source files
target_sources(AudioSender PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        # Add other source files here
)

# Link to JUCE modules
target_link_libraries(AudioSender PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
        # Add other modules as needed
)

# Add include directories for source and shared headers
target_include_directories(AudioSender PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        #${CMAKE_BINARY_DIR}/cmake-build-debug/AudioSender_artefacts/JuceLibraryCode
        #${CMAKE_CURRENT_SOURCE_DIR}/libs/juce/modules
        "/Users/alexanderfortunato/Development/JUCE/Shared Headers"
)

# These definitions are recommended by JUCE.
target_compile_definitions(AudioSender
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)