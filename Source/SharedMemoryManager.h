#pragma once

#include <JuceHeader.h>
#include <atomic>

#define MAX_BUFFER_SIZE 1024 * 1024 // Example size of 1 MB, adjust as needed

struct SharedAudioData
{
    std::atomic<int> writerPosition;
    std::atomic<int> readerPosition;
    std::atomic<bool> isActive;
    std::atomic<int> numChannels;
    std::atomic<int> bufferSize;
    std::atomic<double> sampleRate;
    std::atomic<uint64_t> writeCounter;
    float audioData[MAX_BUFFER_SIZE];
};

class SharedMemoryManager
{
public:
    bool initializeSharedMemory(int numChannels, int bufferSize, double sampleRate);
    void cleanupSharedMemory();

protected:
    int shm_fd = -1;
    SharedAudioData* sharedData = nullptr;
    bool isMemoryInitialized = false;

    static constexpr const char* SHARED_MEMORY_NAME = "/my_shared_audio_buffer";
};

