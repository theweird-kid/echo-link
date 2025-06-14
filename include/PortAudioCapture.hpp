#ifndef PORTAUDIO_CAPTURE_HPP
#define PORTAUDIO_CAPTURE_HPP

#include "ThreadSafeQueue.hpp"
#include "interfaces/IAudioSource.hpp"
#include <portaudio.h>
#include <atomic>

class PortAudioCapture : public IAudioSource
{
private:
    PaStream* m_InputStream = nullptr;
    std::shared_ptr<ThreadSafeQueue<AudioFrame>> m_OutputQueue = nullptr;

    int m_SampleRate;
    int m_Channels;
    int m_FrameSize;
    std::atomic<bool> a_IsRunning;

    // static callback to read PCM from kernel
    static int paInputCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

public:
    PortAudioCapture(int sampleRate, int channels, int frameSize);
    ~PortAudioCapture();

    bool start() override;
    void stop() override;
    void setOutputQueue(std::shared_ptr<ThreadSafeQueue<AudioFrame>> queue) override;

    int getSampleRate() const override { return m_SampleRate; }
    int getChannels() const override { return m_Channels; }
    int getFrameSize() const override { return m_FrameSize; }
};

#endif //PORTAUDIO_CAPTURE_HPP
