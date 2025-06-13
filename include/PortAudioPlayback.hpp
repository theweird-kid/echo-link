#ifndef PORT_AUDIO_PLAYBACK_HPP
#define PORT_AUDIO_PLAYBACK_HPP

#include "ThreadSafeQueue.hpp"
#include "interfaces/IAudioPlayback.hpp"
#include <atomic>
#include <portaudio.h>

class PortAudioPlayback : public IAudioPlayback
{
public:
    PortAudioPlayback(int sampleRate, int channels, int frameSize);
   ~PortAudioPlayback();

   bool start() override;
       void stop() override;
       void setInputQueue(std::shared_ptr<ThreadSafeQueue<AudioFrame>> queue) override { m_InputQueue = std::move(queue); }

       int getSampleRate() const override { return m_SampleRate; }
       int getChannels() const override { return m_Channels; }
       int getFrameSize() const override { return m_FrameSize; }

private:
    std::shared_ptr<ThreadSafeQueue<AudioFrame>> m_InputQueue = nullptr;
    PaStream* m_OutputStream = nullptr;

    int m_SampleRate;
    int m_Channels;
    int m_FrameSize;
    std::atomic_bool a_IsRunning = false;

    // Callback function for PortAudio output stream
    static int paOutputCallback(const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData
    );
};

#endif // PORT_AUDIO_PLAYBACK_HPP
