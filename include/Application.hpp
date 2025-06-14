#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "AudioCodec.hpp"
#include "NetworkManager.hpp"
#include "ThreadSafeQueue.hpp"
#include "interfaces/IAudioPlayback.hpp"
#include "interfaces/IAudioSource.hpp"
#include <asio/io_context.hpp>
#include <memory>

void InitPortAudio();
void TerminatePortAudio();

class Application
{
private:
    asio::io_context m_Context;

    // Modules
    std::unique_ptr<IAudioSource> m_AudioSource;
    std::unique_ptr<IAudioPlayback> m_AudioPlayback;
    std::unique_ptr<AudioCodec> m_AudioCodec;
    std::unique_ptr<NetworkManager> m_NetworkManager;

    // Queues
    std::shared_ptr<ThreadSafeQueue<AudioFrame>> m_CapturedAudioQueue;
    std::shared_ptr<ThreadSafeQueue<std::vector<char>>> m_EncodedAudioQueue;
    std::shared_ptr<ThreadSafeQueue<std::vector<char>>> m_IncomingNetworkQueue;
    std::shared_ptr<ThreadSafeQueue<AudioFrame>> m_DecodedAudioQueue;

    // Threads
    std::thread m_EncodingThread;
    std::thread m_DecodingThread;
    std::thread m_NetworkSendThread;
    std::thread m_AsioRunnerThread;

    // Helper functions
    void encodingLoop();
    void decodingLoop();
    void networkSendLoop();

    bool b_NetworkEnabled;

public:
    Application(int sampleRate, int channels, int frameSize, bool networkEnabled,
        unsigned short localPort, const std::string& remoteIp = "", unsigned short remotePort = 0);

    ~Application();

    void run();
    void stop();
};

#endif // APPLICATION_HPP
