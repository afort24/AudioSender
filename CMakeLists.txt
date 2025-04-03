cmake_minimum_required(VERSION 3.15)
project(AudioSender VERSION 1.0.0)

# Use FetchContent to get JUCE
include(FetchContent)
FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6  # You can specify a specific tag/version here
)
FetchContent_MakeAvailable(JUCE)

# Set up your plugin
juce_add_plugin(AudioSender
        VERSION 1.0.0
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "AudioSender"
        COMPANY_NAME "AlexFortunatoMusic"
)

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

# Add include directories for JuceLibraryCode, source, and shared headers
target_include_directories(AudioSender PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        "/Users/alexanderfortunato/Development/JUCE/Shared Headers"
)