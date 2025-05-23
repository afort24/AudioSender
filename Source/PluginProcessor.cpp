#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "AudioLevelUtils.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>


SlaveAudioSenderAudioProcessor::SlaveAudioSenderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                            .withInput("Main 1/2", juce::AudioChannelSet::stereo(), true)
                            .withInput("1/2", juce::AudioChannelSet::stereo(), true)
                            .withInput("3/4", juce::AudioChannelSet::stereo(), true)
                            .withInput("1",   juce::AudioChannelSet::mono(), true)
                            .withInput("2",   juce::AudioChannelSet::mono(), true)
                            .withInput("3",   juce::AudioChannelSet::mono(), true)
                            .withInput("4",   juce::AudioChannelSet::mono(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
parameters (*this, nullptr, "PARAMETERS", {
    std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("monitor", 1), //<-- jassert error fix
        "Monitor",
        false
    )
})
#endif
{
    DBG("Plugin constructor start");

        monitorParameter = parameters.getRawParameterValue("monitor");

        if (monitorParameter == nullptr)
            DBG("Monitor parameter is NULL! Crash incoming.");
        else
            DBG("Monitor parameter connected.");
}

bool SlaveAudioSenderAudioProcessor::initializeSharedMemory()
{
    // Clean up any existing resources first
    cleanupSharedMemory();

    // First try to unlink any existing shared memory with this name
    // This helps if a previous instance crashed without cleanup
    shm_unlink(SHARED_MEMORY_NAME);

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

    // Cast to shared data structure
    sharedData = static_cast<SharedAudioData*>(mappedMemory);

    // Initialize the shared memory structure with default values
    new (&sharedData->writeIndex) std::atomic<uint64_t>(0);
    new (&sharedData->readIndex) std::atomic<uint64_t>(0);
    new (&sharedData->isActive) std::atomic<bool>(true);
    new (&sharedData->numChannels) std::atomic<int>(currentNumChannels);
    new (&sharedData->bufferSize) std::atomic<int>(currentBlockSize);
    new (&sharedData->sampleRate) std::atomic<double>(currentSampleRate);
    new (&sharedData->sequenceCounter) std::atomic<uint64_t>(0);
    new (&sharedData->maxBufferSize) std::atomic<int>(currentBlockSize);
    new (&sharedData->preferredBufferSize) std::atomic<int>(currentBlockSize);
    new (&sharedData->configurationCounter) std::atomic<uint64_t>(0);
    new (&sharedData->targetLatency) std::atomic<int>(10); // 10ms default
    new (&sharedData->adaptiveBuffering) std::atomic<bool>(true);
    new (&sharedData->metrics.currentLatency) std::atomic<double>(0.0);
    new (&sharedData->metrics.minLatency) std::atomic<double>(1000.0); // Start high
    new (&sharedData->metrics.maxLatency) std::atomic<double>(0.0);
    new (&sharedData->metrics.bufferOverruns) std::atomic<uint64_t>(0);
    new (&sharedData->metrics.bufferUnderruns) std::atomic<uint64_t>(0);

    // Clear the entire ring buffer
    std::memset(sharedData->audioData, 0,
               SharedAudioData::RING_BUFFER_SIZE * 10 * sizeof(float));

    // Clear all block headers
    std::memset(sharedData->blockHeaders, 0,
               SharedAudioData::RING_BUFFER_SIZE * sizeof(SharedAudioData::AudioBlockHeader));

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

void SlaveAudioSenderAudioProcessor::updateBufferSizeIfNeeded()
{
    if (!isMemoryInitialized || sharedData == nullptr)
        return;

    if (sharedData->adaptiveBuffering.load()) {
        // Check if we're getting overruns
        uint64_t overruns = sharedData->metrics.bufferOverruns.load();
        static uint64_t lastOverruns = 0;

        if (overruns > lastOverruns) {
            // Increase buffer size if we're getting overruns
            int currentTarget = sharedData->targetLatency.load();
            sharedData->targetLatency.store(currentTarget + 5); // Add 5ms

            juce::Logger::writeToLog("Increasing target latency to " +
                                   juce::String(currentTarget + 5) + "ms due to buffer overruns");

            lastOverruns = overruns;
        }
    }
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

    // Initialize or reconfigure shared memory with the right parameters
        if (!isMemoryInitialized) {
            initializeSharedMemory();
        } else {
            // Update configuration
            sharedData->sampleRate.store(sampleRate);
            sharedData->preferredBufferSize.store(samplesPerBlock);
            sharedData->numChannels.store(currentNumChannels);
            sharedData->maxBufferSize.store(std::max(sharedData->maxBufferSize.load(), samplesPerBlock));
            sharedData->configurationCounter.fetch_add(1, std::memory_order_release);
        }
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
    // Ensure the output bus is mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Check each input bus
    for (int bus = 0; bus < getBusCount(true); ++bus)
    {
        auto inputSet = layouts.getChannelSet(true, bus);
        if (bus == 0 || bus == 1)  // "1/2" and "3/4" must be stereo
        {
            if (inputSet != juce::AudioChannelSet::stereo())
                return false;
        }
        else  // Buses "1", "2", "3", "4" must be mono
        {
            if (inputSet != juce::AudioChannelSet::mono())
                return false;
        }
    }
    return true;
#endif
}
#endif


void SlaveAudioSenderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Get total channels and number of samples.
    // With multiple buses, getTotalNumInputChannels() should equal 10
    int totalNumInputChannels  = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    // Clear any output channels that didn't contain input data.
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Apply gain to the entire buffer if needed.
    if (gain != 1.0f)
    {
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < numSamples; ++sample)
                channelData[sample] *= gain;
        }
    }

    // Calculate and store the current audio level AFTER applying gain.
    {
        const juce::ScopedLock scopedLock(levelLock);
        currentLevel = AudioLevelUtils::calculateRMSLevel(buffer);
    }

    // Skip further processing if shared memory isn't initialized.
    if (!isMemoryInitialized || sharedData == nullptr)
        return;

    // Update shared memory parameters.
    sharedData->numChannels.store(totalNumInputChannels);
    sharedData->bufferSize.store(numSamples);
    sharedData->sampleRate.store(currentSampleRate);

    // Get current write position in shared memory.
    uint64_t writeIndex = sharedData->writeIndex.load(std::memory_order_acquire);

    // Calculate available space in the ring buffer.
    uint64_t readIndex = sharedData->readIndex.load(std::memory_order_acquire);
    uint64_t available = SharedAudioData::RING_BUFFER_SIZE - (writeIndex - readIndex);

    // Get a sequence number for this block.
    uint64_t sequence = sharedData->sequenceCounter.fetch_add(1, std::memory_order_relaxed);

    // Latency Tracking:
    double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001; // seconds
    double bufferLatency = (available * 1000.0) / currentSampleRate; // in ms
    sharedData->metrics.currentLatency.store(bufferLatency);

    double minLatency = sharedData->metrics.minLatency.load();
    if (bufferLatency < minLatency)
        sharedData->metrics.minLatency.store(bufferLatency);

    double maxLatency = sharedData->metrics.maxLatency.load();
    if (bufferLatency > maxLatency)
        sharedData->metrics.maxLatency.store(bufferLatency);
    // ^ End Latency Tracking

    // Ensure we have enough space to write all samples.
    if (numSamples <= available)
    {
        // Instead of assuming a merged buffer, iterate over each input bus.
        int channelOffset = 0; // Running offset into the interleaved output.
        for (int bus = 0; bus < getBusCount(true); ++bus)
        {
            // Retrieve the buffer for this input bus as a reference.
            const auto& busBuffer = getBusBuffer(buffer, true, bus);
            int numBusChannels = busBuffer.getNumChannels();

            // Write this bus's samples into the shared memory buffer.
            for (int sample = 0; sample < numSamples; ++sample)
            {
                uint64_t frameIndex = (writeIndex + sample) & SharedAudioData::BUFFER_MASK;
                for (int channel = 0; channel < numBusChannels; ++channel)
                {
                    int overallChannel = channelOffset + channel;
                    int bufferIndex = (frameIndex * totalNumInputChannels) + overallChannel;
                    sharedData->audioData[bufferIndex] = busBuffer.getSample(channel, sample);
                }
            }
            channelOffset += numBusChannels;
        }

        // Store block metadata.
        uint64_t headerIndex = writeIndex & SharedAudioData::BUFFER_MASK;
        sharedData->blockHeaders[headerIndex].sequenceNumber = sequence;
        sharedData->blockHeaders[headerIndex].timestamp = juce::Time::getMillisecondCounterHiRes() * 0.001;
        sharedData->blockHeaders[headerIndex].blockSize = numSamples;
        sharedData->blockHeaders[headerIndex].numChannels = totalNumInputChannels;

        // Memory barrier ensures all writes complete before advancing the write index.
        std::atomic_thread_fence(std::memory_order_release);
        sharedData->writeIndex.store(writeIndex + numSamples, std::memory_order_release);
    }
    else
    {
        // Buffer overrun handling.
        juce::Logger::writeToLog("Buffer overrun: needed " + juce::String(numSamples) +
                                   " frames but only " + juce::String(available) + " available");
        sharedData->metrics.bufferOverruns.fetch_add(1, std::memory_order_relaxed);
        // Optionally, you could try to write partial data here.
    }

    // Monitor Button: if monitoring is off, clear the output channels.
    if (monitorParameter != nullptr && monitorParameter->load() < 0.5f)
    {
        for (int i = 0; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, numSamples);
    }

    // Additional functionality: update buffer size if needed.
    updateBufferSizeIfNeeded();
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
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SlaveAudioSenderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin:
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlaveAudioSenderAudioProcessor();
}
