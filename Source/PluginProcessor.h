#pragma once

#include <JuceHeader.h>
#include "SharedMemoryManager.h"


class SlaveAudioSenderAudioProcessor : public juce::AudioProcessor, public SharedMemoryManager
{
public:
    //==============================================================================
    SlaveAudioSenderAudioProcessor();
    ~SlaveAudioSenderAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // new:
    bool initializeSharedMemory();
    void cleanupSharedMemory();
    
    //==============================================================================
    //UI Parameters getter
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }
    
    //Meter/fader stuff:
    // Set the gain multiplier (linear)
        void setGain(float newGain)
        {
            gain = newGain;
        }
        
        // Get the current gain multiplier (linear)
        float getGain() const
        {
            return gain;
        }
        
        // Get the current audio level in dB
        float getCurrentLevel() const
        {
            return currentLevel;
        }
        
        // Returns true if shared memory is initialized and active
        bool isMemoryInitializedAndActive() const
        {
            return isMemoryInitialized && sharedData != nullptr && sharedData->isActive.load();
        }

    
    
private:
    
    static constexpr const char* SHARED_MEMORY_NAME = "/my_shared_audio_buffer";
    double currentSampleRate = 0.0;
    int currentBlockSize = 0;
    int currentNumChannels = 0;
    void updateBufferSizeIfNeeded();
    
    // UI Parameters:
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* monitorParameter = nullptr;
    
    // Gain control and metering
    float gain = 1.0f;           // Linear gain (1.0 = unity gain)
    float currentLevel = -60.0f;  // Current level in dB
      
    // Lock for ensuring thread safety when updating the level
    juce::CriticalSection levelLock;
    

    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlaveAudioSenderAudioProcessor)
};

