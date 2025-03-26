#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <new> // For alignas

#define MAX_BUFFER_SIZE 1024 * 1024 // Example size of 1 MB, adjust as needed

// In SharedMemoryManager.h, modify SharedAudioData structure:
struct alignas (64) SharedAudioData
{
    std::atomic<uint64_t> writeIndex;    // Where producer writes next
    std::atomic<uint64_t> readIndex;     // Where consumer reads next
    std::atomic<bool> isActive;
    std::atomic<int> numChannels;
    std::atomic<int> bufferSize;
    std::atomic<double> sampleRate;
    
    // Buffer size in frames (not bytes)
    static constexpr uint32_t RING_BUFFER_SIZE = 8192; // Power of 2 is important!
    static constexpr uint32_t BUFFER_MASK = RING_BUFFER_SIZE - 1;
    
    // Audio data now organized as a ring buffer
    // Each frame has multiple samples (one per channel)
    float audioData[RING_BUFFER_SIZE * 8]; // Support up to 8 channels
    
    struct AudioBlockHeader {
        uint64_t sequenceNumber;
        double timestamp;
        int blockSize;
        int numChannels;
    };

    std::atomic<uint64_t> sequenceCounter;
    AudioBlockHeader blockHeaders[RING_BUFFER_SIZE]; // One header per potential block
    
    std::atomic<int> maxBufferSize;
    std::atomic<int> preferredBufferSize;
    std::atomic<uint64_t> configurationCounter; // Incremented when settings change
    
    struct LatencyMetrics {
        std::atomic<double> currentLatency; // in milliseconds
        std::atomic<double> minLatency;
        std::atomic<double> maxLatency;
        std::atomic<uint64_t> bufferOverruns;
        std::atomic<uint64_t> bufferUnderruns;
    };
    // In your UI, add methods to display these metrics
    LatencyMetrics metrics;
  
    
    std::atomic<int> targetLatency; // in milliseconds
    std::atomic<bool> adaptiveBuffering;

    // Add logic to adjust buffer size based on underruns/overruns
    // This would monitor performance and adjust buffer sizes dynamically

};



class SharedMemoryManager
{
public:
//    bool initializeSharedMemory(int numChannels, int bufferSize, double sampleRate);
    bool initializeSharedMemory();
    void cleanupSharedMemory();

protected:
    int shm_fd = -1;
    SharedAudioData* sharedData = nullptr;
    bool isMemoryInitialized = false;

    static constexpr const char* SHARED_MEMORY_NAME = "/my_shared_audio_buffer";
};



