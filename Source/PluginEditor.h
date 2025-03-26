#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


class SlaveAudioSenderAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SlaveAudioSenderAudioProcessorEditor (SlaveAudioSenderAudioProcessor&);
    ~SlaveAudioSenderAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    SlaveAudioSenderAudioProcessor& audioProcessor;
    
    juce::ToggleButton monitorButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monitorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlaveAudioSenderAudioProcessorEditor)
};

