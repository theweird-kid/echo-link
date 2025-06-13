#ifndef FAKE_AUDIO_SOURCE_HPP
#define FAKE_AUDIO_SOURCE_HPP

#include "./interfaces/IAudioSource.hpp"
#include <atomic>
#include <thread>
#include <fstream>
#include <string>

class FakeAudioSource : public IAudioSource
{
private:
    std::string m_FilePath;
    std::ifstream m_AudioFile;
    std::shared_ptr<ThreadSafeQueue<AudioFrame>> m_OutputQueue = nullptr;

    std::thread m_ReadThread;
    std::atomic_bool a_IsRunning = false;

    int m_SampleRate;
    int m_Channels;
    int m_FrameSize;

    // readLoop runs in m_ReadThread
    void readLoop();

public:
    // Constructor
    FakeAudioSource(const std::string& filePath, int sampleRate, int channels, int frameSize);
    // Destructor
    ~FakeAudioSource();

    bool start() override;
    void stop() override;
    void setOutputQueue(std::shared_ptr<ThreadSafeQueue<AudioFrame>> queue) override;

    // Basic getters
    int getSampleRate() const override { return m_SampleRate; }
    int getChannels() const override { return m_Channels; }
    int getFrameSize() const override { return m_FrameSize; }
};

#endif // FAKE_AUDIO_SOURCE_HPP
