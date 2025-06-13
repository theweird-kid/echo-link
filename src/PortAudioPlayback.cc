#include "PortAudioPlayback.hpp"
#include <iostream>
#include <portaudio.h>

PortAudioPlayback::PortAudioPlayback(int sampleRate, int channels, int frameSize)
    : m_SampleRate(sampleRate), m_Channels(channels), m_FrameSize(frameSize)
{
    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        std::cerr << "PortAudio initialization error: " << Pa_GetErrorText(err) << std::endl;
        exit(1);
    }
}

PortAudioPlayback::~PortAudioPlayback()
{
    // Terminate PortAudio
    stop();
}

bool PortAudioPlayback::start()
{
    if(m_InputQueue == nullptr) {
        std::cerr << "[PortAudioPlayback] Error: Input queue is not initialized." << std::endl;
        return false;
    }
    if(a_IsRunning.load()) {
        std::cerr << "[PortAudioPlayback] Error: Already running." << std::endl;
        return false;
    }

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if(outputParameters.device == paNoDevice) {
        std::cerr << "[PortAudioPlayback] Error: No default output device found." << std::endl;
        return false;
    }

    outputParameters.channelCount = m_Channels;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &m_OutputStream,
        NULL,
        &outputParameters,
        m_SampleRate,
        m_FrameSize,
        paNoFlag,
        paOutputCallback,
        this
    );
    if(err != paNoError) {
        std::cerr << "[PortAudioPlayback] Error: Failed to open PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
        m_OutputStream = nullptr;
        return false;
    }

    err = Pa_StartStream(m_OutputStream);
    if(err != paNoError) {
        std::cerr << "[PortAudioPlayback] Error: Failed to start PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(m_OutputStream);
        m_OutputStream = nullptr;
        return false;
    }

    a_IsRunning.store(true);
    std::cout << "[PortAudioPlayback] Started playback (SR: " << m_SampleRate << ", CH: " << m_Channels << ", Frame: " << m_FrameSize << ")" << std::endl;
    return true;
}
