#include "PluginProcessor.h"
#include "PluginEditor.h"

//Posix Headers
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

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
    // Initialize shared memory in prepareToPlay instead of constructor
    // This ensures we have proper audio parameters
}

bool SlaveAudioSenderAudioProcessor::initializeSharedMemory()
{
    // Clean up any existing resources first
    cleanupSharedMemory();
    
    // Create or open shared memory
    shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    
    if (shm_fd == -1)
    {
        juce::Logger::writeToLog("Failed to create shared memory: " + juce::String(strerror(errno)));
        return false;
    }

    // Set the size of the shared memory segment
    if (ftruncate(shm_fd, MAX_BUFFER_SIZE) == -1)
    {
        juce::Logger::writeToLog("Failed to set shared memory size: " + juce::String(strerror(errno)));
        close(shm_fd);
        shm_fd = -1;
        return false;
    }

    // Map the shared memory into our address space
    void* mappedMemory = mmap(0, MAX_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    if (mappedMemory == MAP_FAILED)
    {
        juce::Logger::writeToLog("Failed to map shared memory: " + juce::String(strerror(errno)));
        close(shm_fd);
        shm_fd = -1;
        return false;
    }

    // Cast to our shared data structure
    sharedData = static_cast<SharedAudioData*>(mappedMemory);
    
    // Initialize the shared memory structure with default values
    new (&sharedData->writerPosition) std::atomic<int>(0);
    new (&sharedData->readerPosition) std::atomic<int>(0);
    new (&sharedData->isActive) std::atomic<bool>(true);
    new (&sharedData->numChannels) std::atomic<int>(currentNumChannels);
    new (&sharedData->bufferSize) std::atomic<int>(currentBlockSize);
    new (&sharedData->sampleRate) std::atomic<double>(currentSampleRate);
    new (&sharedData->writeCounter) std::atomic<uint64_t>(0);
    
    // Clear the audio buffer
    std::memset(sharedData->audioData, 0, currentBlockSize * currentNumChannels * sizeof(float));
    
    juce::Logger::writeToLog("Shared memory initialized successfully at address: " +
                            juce::String(reinterpret_cast<uintptr_t>(sharedData)));
    
    isMemoryInitialized = true;
    return true;
}

void SlaveAudioSenderAudioProcessor::cleanupSharedMemory()
{
    if (sharedData != nullptr)
    {
        // Set inactive flag before unmapping to notify receivers
        if (isMemoryInitialized)
        {
            sharedData->isActive.store(false);
        }
        
        munmap(sharedData, MAX_BUFFER_SIZE);
        sharedData = nullptr;
    }

    if (shm_fd != -1)
    {
        close(shm_fd);
        shm_unlink(SHARED_MEMORY_NAME);
        shm_fd = -1;
    }
    
    isMemoryInitialized = false;
}

//Destructor
SlaveAudioSenderAudioProcessor::~SlaveAudioSenderAudioProcessor()
{
    cleanupSharedMemory();
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
    // Store the audio parameters
    currentSampleRate = getSampleRate();
    currentBlockSize = samplesPerBlock;
    currentNumChannels = getTotalNumInputChannels();
    
    // Initialize shared memory with the right parameters
    initializeSharedMemory();
}

void SlaveAudioSenderAudioProcessor::releaseResources()
{
    cleanupSharedMemory();
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

    // Clear any output channels that didn't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Skip processing if shared memory isn't initialized
    if (!isMemoryInitialized || sharedData == nullptr)
        return;
        
    // Update shared memory with current parameters (in case they changed)
    sharedData->numChannels.store(totalNumInputChannels);
    sharedData->bufferSize.store(buffer.getNumSamples());
    sharedData->sampleRate.store(currentSampleRate);
    
    // Get writerPosition - this is where we'll write our data
    int writePos = 0; // Always start at beginning of buffer for simplicity
    
    // Write audio data in interleaved format (easier for most receivers to handle)
    int numSamples = buffer.getNumSamples();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            int index = (sample * totalNumInputChannels) + channel;
            sharedData->audioData[index] = buffer.getSample(channel, sample);
        }
    }
    
    // Increment write counter to notify receiver that new data is available
    sharedData->writeCounter.fetch_add(1);
    
    // Pass through audio unchanged
    // If you want to mute the output, you can uncomment the next line
    // buffer.clear();
}

//==============================================================================
bool SlaveAudioSenderAudioProcessor::hasEditor() const
{
    return true;
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
