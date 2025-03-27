#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AudioLevelUtils.h"

//==============================================================================
SlaveAudioSenderAudioProcessorEditor::SlaveAudioSenderAudioProcessorEditor (SlaveAudioSenderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    //Monitor button:
    monitorButton.setButtonText("Monitor");
    monitorButton.setTooltip("Enable to monitor audio in sender DAW");
    // Create the attachment to link the button to the parameter
    monitorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getValueTreeState(), "monitor", monitorButton);
    addAndMakeVisible(monitorButton);
    
    // Add and configure the meter/fader component
    addAndMakeVisible(meterFader);
    meterFader.addListener(this);  // Register as a listener for gain changes
    meterFader.setGain(0.0f);      // Set initial gain to 0 dB
    
    // Start timer for updating the meter
    startTimerHz(24);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

SlaveAudioSenderAudioProcessorEditor::~SlaveAudioSenderAudioProcessorEditor()
{
    stopTimer();
    meterFader.removeListener(this);
}

//==============================================================================
void SlaveAudioSenderAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font(15.0f));
    g.drawFittedText("Audio Sender Plugin", getLocalBounds().removeFromTop(30), juce::Justification::centred, 1);
}

void SlaveAudioSenderAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto area = getLocalBounds();
    
    // Position the title
    area.removeFromTop(30);
    
    // Position the monitor button
    monitorButton.setBounds(area.removeFromTop(30).withSizeKeepingCentre(150, 24));
    
    // Position the meter/fader component
    int meterWidth = 80;
    int margin = 20;
    meterFader.setBounds(area.getCentreX() - meterWidth/2, area.getY() + margin,
                       meterWidth, area.getHeight() - margin*2);
}

void SlaveAudioSenderAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    // Update processor's gain parameter when the slider changes
    if (slider == &meterFader.getSlider())
    {
        float gainValue = meterFader.getGainLinear();
        audioProcessor.setGain(gainValue);
    }
}

void SlaveAudioSenderAudioProcessorEditor::timerCallback()
{
    // Update the meter with the current audio level
    float currentLevel = audioProcessor.getCurrentLevel();
    meterFader.setLevel(currentLevel);
}
