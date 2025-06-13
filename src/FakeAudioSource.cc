#include "FakeAudioSource.hpp"
#include "interfaces/IAudioPlayback.hpp"
#include "opus_types.h"

#include <chrono>
#include <iostream>
#include <thread>

FakeAudioSource::FakeAudioSource(const std::string& filePath, int sampleRate, int channels, int frameSize)
    : m_FilePath(filePath), m_SampleRate(sampleRate), m_Channels(channels), m_FrameSize(frameSize)
{
    // Open the audio file
    m_AudioFile.open(m_FilePath, std::ios::binary);
    if(!m_AudioFile.is_open()) {
        std::cerr << "[FakeAudioSource] Failed to open audio file: " << m_FilePath << std::endl;
    } else {
        std::cout << "[FakeAudioSource] Successfully opened audio file: " << m_FilePath << std::endl;
    }
}

FakeAudioSource::~FakeAudioSource()
{
    stop();
    if(m_AudioFile.is_open()) {
        m_AudioFile.close();
    }
}

void FakeAudioSource::setOutputQueue(std::shared_ptr<ThreadSafeQueue<AudioFrame>> queue)
{
    m_OutputQueue = std::move(queue);
}

bool FakeAudioSource::start()
{
    if(!m_AudioFile.is_open()) {
        std::cerr << "[FakeAudioSource] Error: Audio File not open\n";
        return false;
    }
    if(m_OutputQueue == nullptr) {
        std::cerr << "[FakeAudioSource] Error: m_OutputQueue is NULL\n";
        return false;
    }
    if(a_IsRunning.load()) {
        std::cout << "[FakeAudioSource] Already running\n";
        return true;
    }

    a_IsRunning.store(true);
    m_ReadThread = std::thread(&FakeAudioSource::readLoop, this);
    std::cout << "[FakeAudioSource] Reading Thread started\n";
    return true;
}

void FakeAudioSource::stop()
{
    if(a_IsRunning.load()) {
        a_IsRunning.store(false);
        if(m_ReadThread.joinable()) {
            m_ReadThread.join();
        }

        std::cout << "[FakeAudioSource] Reading thread stopped\n";
    }
}

void FakeAudioSource::readLoop()
{
    const int bytesPerSample = sizeof(opus_int16);
    const int bytesPerFrame = m_FrameSize * m_Channels * bytesPerSample;

    const std::chrono::duration<double> frameDuration =
        std::chrono::duration<double>((double)m_FrameSize / m_SampleRate);

    const auto sleepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameDuration);

    std::cout << "[FakeAudioSource] Read Loop started. Bytes per frame: " << bytesPerFrame
        << ", Simulated sleep duration: " << sleepDuration.count() << "ms" << std::endl;

    m_AudioFile.clear();
    m_AudioFile.seekg(0, std::ios::beg);

    while(a_IsRunning.load())
    {
        AudioFrame currentFrame(m_FrameSize * m_Channels);
        // Read audio data from file into internal buffer
        m_AudioFile.read(reinterpret_cast<char*>(currentFrame.data()), bytesPerFrame);

        if(m_AudioFile.gcount() == 0) {
            std::cout << "[FakeAudioSource] End of file reached. Looping back to start\n";
            m_AudioFile.clear();
            m_AudioFile.seekg(0, std::ios::beg);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        } else if(m_AudioFile.gcount() < bytesPerFrame) {
            std::cerr << "[FakeAudioSource] Warning: Partial frame read. "
            << m_AudioFile.gcount() << " bytes read" << std::endl;

            std::fill(currentFrame.begin() + (m_AudioFile.gcount() / bytesPerSample), currentFrame.end(), 0);
        }

        // Check if the output queue is shutting down before pushing
        if (m_OutputQueue->is_shutting_down()) {
            std::cout << "[FakeAudioSource] Output queue signalled shutdown. Exiting read loop." << std::endl;
            break; // Exit the loop if the destination queue is no longer accepting data
        }

        m_OutputQueue->push(currentFrame); // Push the read audio frame to the queue
        // std::cout << "[FakeAudioSource] Pushed frame. Queue size: " << _outputQueue->size() << std::endl; // For debugging

        // Simulate real-time capture delay
        std::this_thread::sleep_for(sleepDuration);
    }

    std::cout << "[FakeAudioSource] Read loop exited." << std::endl;
}
