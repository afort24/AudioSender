#include "PluginProcessor.h"
#include "PluginEditor.h"

//Posix Headers
//Git Test 7
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

//==============================================================================

//Constructor
SlaveAudioSenderAudioProcessor::SlaveAudioSenderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    //Create Shared Memory
    shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    
    if (shm_fd == -1)
    {
        juce::Logger::writeToLog ("Failed to create shared memory: " + juce::String (strerror(errno)));
        return;
    }

    ftruncate(shm_fd, BUFFER_SIZE);
    void* sharedMemory = mmap(0, BUFFER_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    if (sharedMemory == MAP_FAILED)
    {
        juce::Logger::writeToLog("Failed to map shared memory");
        return;
    }

    audioBuffer = static_cast<float*>(sharedMemory);
    
    juce::Logger::writeToLog("Shared memory created at address: " + juce::String((uintptr_t)audioBuffer));

}

//Destructor
SlaveAudioSenderAudioProcessor::~SlaveAudioSenderAudioProcessor()
{
    if (audioBuffer != nullptr)
    {
        munmap(audioBuffer, BUFFER_SIZE);
        shm_unlink(SHARED_MEMORY_NAME);
    }
}

//==============================================================================
const juce::String SlaveAudioSenderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SlaveAudioSenderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SlaveAudioSenderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SlaveAudioSenderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SlaveAudioSenderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SlaveAudioSenderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SlaveAudioSenderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SlaveAudioSenderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SlaveAudioSenderAudioProcessor::getProgramName (int index)
{
    return {};
}

void SlaveAudioSenderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SlaveAudioSenderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
}

void SlaveAudioSenderAudioProcessor::releaseResources()
{
    if (audioBuffer != nullptr)
    {
        munmap(audioBuffer, BUFFER_SIZE);
        audioBuffer = nullptr;
    }

    if (shm_fd != -1)
    {
        close(shm_fd);
        shm_unlink(SHARED_MEMORY_NAME);
        shm_fd = -1;
    }
}



#ifndef JucePlugin_PreferredChannelConfigurations
bool SlaveAudioSenderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SlaveAudioSenderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels beyond the input channels to avoid garbage
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (audioBuffer != nullptr)
    {
        // ✅ Clear shared buffer first (optional but clean)
        std::fill(audioBuffer, audioBuffer + numSamples * numChannels, 0.0f);

        int maxSamples = BUFFER_SIZE / sizeof(float) / numChannels;
        int numSamplesToWrite = std::min(numSamples, maxSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamplesToWrite; ++i)
            {
                // ✅ Write in channel-first order (non-interleaved format)
                audioBuffer[ch * numSamples + i] = channelData[i];
            }
        }
    }
}



//==============================================================================
bool SlaveAudioSenderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SlaveAudioSenderAudioProcessor::createEditor()
{
    return new SlaveAudioSenderAudioProcessorEditor (*this);
}

//==============================================================================
void SlaveAudioSenderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SlaveAudioSenderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlaveAudioSenderAudioProcessor();
}

