#include "Application.hpp"
#include "NetworkManager.hpp"
#include "PortAudioCapture.hpp"
#include "PortAudioPlayback.hpp"
#include "opus_defines.h"

#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <portaudio.h>
#include <stdexcept>
#include <vector>

// Port Audio status flags
static bool g_Pa_Initialized = false;
static int g_Pa_RefCount = 0;
std::mutex g_Pa_Mutex;

void InitPortAudio()
{
    std::lock_guard<std::mutex> lock(g_Pa_Mutex);
    if(!g_Pa_Initialized)
    {
        PaError err = Pa_Initialize();
        if(err != paNoError)
        {
            throw std::runtime_error("Failed to initialize PortAudio: " + std::string(Pa_GetErrorText(err)));
        }
        g_Pa_Initialized = true;
        std::cout << "[PortAudio Global] initialized successfully." << std::endl;
    }
    g_Pa_RefCount++;
}

void TerminatePortAudio()
{
    std::lock_guard<std::mutex> lock(g_Pa_Mutex);
    g_Pa_RefCount--;
    if(g_Pa_RefCount == 0 && g_Pa_Initialized)
    {
        PaError err = Pa_Terminate();
        if(err != paNoError)
        {
            throw std::runtime_error("Failed to terminate PortAudio: " + std::string(Pa_GetErrorText(err)));
        }
        g_Pa_Initialized = false;
        std::cout << "[PortAudio Global] terminated successfully." << std::endl;
    }
}

Application::Application(int sampleRate, int channels, int frameSize, bool networkEnabled,
    unsigned short localPort, const std::string& remoteIp, unsigned short remotePort)

    : b_NetworkEnabled(networkEnabled),
    m_AudioCodec(std::make_unique<AudioCodec>()),
    m_CapturedAudioQueue(std::make_shared<ThreadSafeQueue<AudioFrame>>()),
    m_EncodedAudioQueue(std::make_shared<ThreadSafeQueue<std::vector<char>>>()),
    m_IncomingNetworkQueue(std::make_shared<ThreadSafeQueue<std::vector<char>>>()),
    m_DecodedAudioQueue(std::make_shared<ThreadSafeQueue<AudioFrame>>())
{
    // Init Port Audio
    InitPortAudio();

    // Initialize Audio Source
    m_AudioSource = std::make_unique<PortAudioCapture>(sampleRate, channels, frameSize);
    std::cout << "[Application] Using PortAudioCapture (live microphone) for input." << std::endl;
    m_AudioSource->setOutputQueue(m_CapturedAudioQueue);

    // Initialize Audio Playback
    m_AudioPlayback = std::make_unique<PortAudioPlayback>(sampleRate, channels, frameSize);
    m_AudioPlayback->setInputQueue(m_DecodedAudioQueue);

    // Initialize Audio Codec
    if(!m_AudioCodec->initEncoder(sampleRate, channels, OPUS_APPLICATION_VOIP)) {
        throw std::runtime_error("Failed to initialize Opus Encoder.");
    }
    if(!m_AudioCodec->initDecoder(sampleRate, channels)) {
        throw std::runtime_error("Failed to initialize Opus Decoder.");
    }

    // Initialize Network
    if(b_NetworkEnabled) {
        m_WorkGuard.emplace(m_Context.get_executor());

        m_NetworkManager = std::make_unique<NetworkManager>(m_Context);
        if(!m_NetworkManager->init(localPort)) {
            throw std::runtime_error("Failed to initialize NetworkManager.");
        }
        m_NetworkManager->setRemoteEndpoint(remoteIp, remotePort);
        m_NetworkManager->setIncomingQueue(m_IncomingNetworkQueue);

        // Run Asio thread
        m_AsioRunnerThread = std::thread([this](){
            std::cout << "[AsioRunner] io_context runner thread started." << std::endl;
            try {
                m_Context.run();
            } catch(const std::exception &e) {
                std::cerr << "[AsioRunner] io_context error: " << e.what() << std::endl;
            }
            std::cout << "[AsioRunner] io_context runner thread stopped." << std::endl;
        });

        m_NetworkSendThread = std::thread(&Application::networkSendLoop, this);
    }

    // Start encodin and decoding threads
    m_EncodingThread = std::thread(&Application::encodingLoop, this);
    m_DecodingThread = std::thread(&Application::decodingLoop, this);

    std::cout << "[Application] Setup Complete." << std::endl;
}

Application::~Application()
{
    stop();
    TerminatePortAudio();
}

void Application::run()
{
    std::cout << "[Application] Running.." << std::endl;

    if(!m_AudioSource->start()) {
        std::cerr << "[Application] Failed to start audio source. Exiting\n";
        stop();
        return;
    }

    if (!m_AudioPlayback->start()) {
        std::cerr << "[Application] Failed to start audio playback. Exiting." << std::endl;
        stop();
        return;
    }

    if (b_NetworkEnabled && m_NetworkManager) {
        m_NetworkManager->startReceive();
    }

    std::string line;
    std::cout << "Type 'exit' to stop." << std::endl;
    while (std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        }
    }
    std::cout << "Exit command received." << std::endl;
}

void Application::stop() {
    std::cout << "[Application] Stopping..." << std::endl;

    m_CapturedAudioQueue->Shutdown();
    m_EncodedAudioQueue->Shutdown();
    m_IncomingNetworkQueue->Shutdown();
    m_DecodedAudioQueue->Shutdown();

    m_AudioSource->stop();
    m_AudioPlayback->stop();

    if (b_NetworkEnabled && m_NetworkManager) {
        m_NetworkManager->stop();
        if (m_WorkGuard.has_value()) {
            m_WorkGuard->reset(); // This signals io_context.run() to stop if no other work is pending
        }
        m_Context.stop();
    }

    if (m_EncodingThread.joinable()) {
        m_EncodingThread.join();
    }
    if (m_DecodingThread.joinable()) {
        m_DecodingThread.join();
    }
    if (b_NetworkEnabled) {
        if (m_NetworkSendThread.joinable()) {
            m_NetworkSendThread.join();
        }
        if (m_AsioRunnerThread.joinable()) {
            m_AsioRunnerThread.join();
        }
    }

    std::cout << "[Application] Stopped." << std::endl;
}

void Application::encodingLoop() {
    std::cout << "[Encoding Thread] Started." << std::endl;
    const int maxOpusPacketSize = 4000;
    std::vector<char> opusPacket(maxOpusPacketSize);

    while (true) {
        AudioFrame rawFrame;
        if (!m_CapturedAudioQueue->pop(rawFrame)) {
            std::cout << "[Encoding Thread] Source audio queue shut down. Exiting." << std::endl;
            break;
        }

        if (rawFrame.size() != static_cast<size_t>(m_AudioSource->getFrameSize() * m_AudioSource->getChannels())) {
            std::cerr << "[Encoding Thread] Warning: Raw frame size mismatch (" << rawFrame.size()
                << " vs expected " << (m_AudioSource->getFrameSize() * m_AudioSource->getChannels())
                << "). Skipping frame." << std::endl;
            continue;
        }

        int encodedBytes = m_AudioCodec->encode(rawFrame.data(), m_AudioSource->getFrameSize(),
            reinterpret_cast<unsigned char*>(opusPacket.data()), maxOpusPacketSize);

        if (encodedBytes < 0) {
            std::cerr << "[Encoding Thread] Opus encoding error: " << encodedBytes << std::endl;
            continue;
        }

        NetworkPacket packet(opusPacket.data(), opusPacket.data() + encodedBytes);

        if (b_NetworkEnabled && m_NetworkManager) {
            //m_NetworkManager->sendPacket(packet);
            m_EncodedAudioQueue->push(packet);
        } else {
            m_IncomingNetworkQueue->push(packet);
        }
    }
    std::cout << "[Encoding Thread] Exited." << std::endl;
}

void Application::decodingLoop() {
    std::cout << "[Decoding Thread] Started." << std::endl;
    AudioFrame decodedPcm(
        m_AudioSource->getFrameSize() * m_AudioSource->getChannels()
    );

    while (true) {
        NetworkPacket encodedPacket;
        if (!m_IncomingNetworkQueue->pop(encodedPacket)) {
            std::cout << "[Decoding Thread] Incoming packet queue shut down. Exiting." << std::endl;
            break;
        }

        int decodedSamples = m_AudioCodec->decode(
            reinterpret_cast<const unsigned char*>(encodedPacket.data()),
            encodedPacket.size(),
            decodedPcm.data(),
            m_AudioSource->getFrameSize()
        );

        if (decodedSamples < 0) {
            std::cerr << "[Decoding Thread] Opus decoding error: " << decodedSamples << std::endl;
            continue;
        }

        m_DecodedAudioQueue->push(AudioFrame(decodedPcm.begin(), decodedPcm.begin() + (decodedSamples * m_AudioSource->getChannels())));
    }
    std::cout << "[Decoding Thread] Exited." << std::endl;
}

void Application::networkSendLoop() {
    if (!b_NetworkEnabled || !m_NetworkManager) {
        return;
    }
    std::cout << "[Network Send Thread] Started." << std::endl;
    while (true) {
        NetworkPacket packetToSend;
        if (m_EncodedAudioQueue->pop(packetToSend)) {
            m_NetworkManager->sendPacket(packetToSend);
        }
    }
    std::cout << "[Network Send Thread] Exited." << std::endl;
}
