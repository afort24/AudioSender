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
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SlaveAudioSenderAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlaveAudioSenderAudioProcessorEditor)
};

