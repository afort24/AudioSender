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
    
    // Add and configure connection status label
    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    statusLabel.setText("Checking connection...", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    
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
    // Gradient background (black to dark grey)
    juce::ColourGradient backgroundGradient(
        juce::Colour::fromRGB(30, 30, 30), // Top color
        0, 0,
        juce::Colours::black,              // Bottom color
        0, static_cast<float>(getHeight()),
        false
    );
    backgroundGradient.addColour(0.8, juce::Colours::black);
    g.setGradientFill(backgroundGradient);
    g.fillRect(getLocalBounds());

    // Title Text (centered at top)
    g.setColour(juce::Colours::goldenrod);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    juce::Rectangle<int> titleArea = getLocalBounds().removeFromTop(40).reduced(0, 10);
    g.drawFittedText("Audio Sender Plugin", titleArea, juce::Justification::centred, 1);

    // ===============================
    // Footer Text at Bottom:
    // ===============================

    // Line 1: Copyright-safe UTF-8
    const char* utf8Text1 = u8"Alex Fortunato Music ©";
    juce::String footerText1 = juce::String::fromUTF8(utf8Text1, static_cast<int>(std::strlen(utf8Text1)));
    juce::String footerText2 = " 2025";
    juce::String websiteText = "alexfortunatomusic.com";

    juce::String footerLine1 = footerText1 + footerText2;
    juce::String footerLine2 = websiteText;

    g.setFont(juce::Font(12.0f));
    g.setColour(juce::Colours::goldenrod);

    auto bounds = getLocalBounds().reduced(10);
    auto footerArea = bounds.removeFromBottom(30);

    juce::Rectangle<int> line1 = footerArea.removeFromTop(15);
    juce::Rectangle<int> line2 = footerArea;

    g.drawFittedText(footerLine1, line1, juce::Justification::centred, 1);
    g.drawFittedText(footerLine2, line2, juce::Justification::centred, 1);
}


void SlaveAudioSenderAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Reserve space for footer
    auto footerArea = area.removeFromBottom(30);

    // ===== Title Area =====
    area.removeFromTop(30); // space for title text (drawn in paint)

    // ===== Status Label =====
    statusLabel.setBounds(area.removeFromTop(24).reduced(20, 0));

    // ===== Monitor Button (centered) =====
    int buttonWidth = 150;
    int buttonHeight = 24;
    monitorButton.setBounds(
        (getWidth() - buttonWidth) / 2,
        (getHeight() - buttonHeight) / 2,
        buttonWidth,
        buttonHeight
    );

    // ===== Meter/Fader in Bottom-Right =====
    int meterWidth = 70;
    int meterHeight = 110;
    int padding = 10;

    meterFader.setBounds(
        getWidth() - meterWidth - padding,
        getHeight() - meterHeight - 30 - padding, // keep it above the footer
        meterWidth,
        meterHeight
    );
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
    // Meter update
    float currentLevel = audioProcessor.getCurrentLevel();
    meterFader.setLevel(currentLevel);

    // Connection status update
    if (audioProcessor.isMemoryInitializedAndActive())
    {
        statusLabel.setText("Connected to AudioReceiver", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lime);
    }
    else
    {
        statusLabel.setText("Not Connected", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
}
