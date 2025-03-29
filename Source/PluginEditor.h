#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AudioMeterFader.h"

class SlaveAudioSenderAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Slider::Listener,
                                          private juce::Timer
{
public:
    SlaveAudioSenderAudioProcessorEditor (SlaveAudioSenderAudioProcessor&);
    ~SlaveAudioSenderAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    // Slider::Listener implementation
    void sliderValueChanged(juce::Slider* slider) override;
    
    // Timer callback to update the meter
    void timerCallback() override;

private:

    SlaveAudioSenderAudioProcessor& audioProcessor;
    
    // Monitor button (keep your existing control)
    juce::ToggleButton monitorButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monitorAttachment;
    
    // Audio meter and gain fader component
    AudioMeterFader meterFader;
    
    // Connection status label
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlaveAudioSenderAudioProcessorEditor)
};
