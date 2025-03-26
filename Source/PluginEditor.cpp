#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SlaveAudioSenderAudioProcessorEditor::SlaveAudioSenderAudioProcessorEditor (SlaveAudioSenderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    //Monitor button:
    monitorButton.setButtonText("Monitor");
    monitorButton.setTooltip("Enable to monitor audio in sender DAW");
    // Create the attachment to link the button to the parameter
    monitorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "monitor", monitorButton);
    addAndMakeVisible(monitorButton);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

SlaveAudioSenderAudioProcessorEditor::~SlaveAudioSenderAudioProcessorEditor()
{
}

//==============================================================================
void SlaveAudioSenderAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText("Audio Sender Plugin", getLocalBounds().removeFromTop(30), juce::Justification::centred, 1);
}

void SlaveAudioSenderAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    // Position the monitor button
        auto area = getLocalBounds();
        area.removeFromTop(50); // Leave space at the top
        monitorButton.setBounds(area.removeFromTop(30).withSizeKeepingCentre(150, 24));
}

