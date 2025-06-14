#include "PortAudioPlayback.hpp"
#include "interfaces/IAudioPlayback.hpp"

#include "opus_types.h"
#include <iostream>
#include <portaudio.h>

PortAudioPlayback::PortAudioPlayback(int sampleRate, int channels, int frameSize)
    : m_SampleRate(sampleRate), m_Channels(channels), m_FrameSize(frameSize)
{
    // Initialize PortAudio in the global scope, to avoid multiple initializations
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

void PortAudioPlayback::stop()
{
    if(a_IsRunning.load()) {
        a_IsRunning.store(false);
        if(m_OutputStream != nullptr) {
            PaError err = Pa_StopStream(m_OutputStream);
            if(err != paNoError) {
                std::cerr << "[PortAudioPlayback] Error: Failed to stop PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            }

            err = Pa_CloseStream(m_OutputStream);
            if(err != paNoError) {
                std::cerr << "[PortAudioPlayback] Error: Failed to close PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            }

            m_OutputStream = nullptr;
        }
        std::cout << "[PortAudioPlayback] Stopped playback" << std::endl;
    }
}

int PortAudioPlayback::paOutputCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
)
{
    PortAudioPlayback* self = static_cast<PortAudioPlayback*>(userData);
    opus_int16* out = static_cast<opus_int16*>(outputBuffer);
    unsigned long framesToFill = framesPerBuffer;
    int samplesToFill = framesToFill * self->m_Channels;

    AudioFrame decodedFrame;
    // Decode audio from input Queue
    if(!self->m_InputQueue->try_pop(decodedFrame)) {
        std::fill(out, out + samplesToFill, 0);
        if(self->m_InputQueue->is_shutting_down() && self->m_InputQueue->empty()) {
            return paComplete;
        }
        // Can maybe add jitter buffer
        return paContinue;
    }

    // Copy decoded samples to output buffer
    size_t samplesInFrame = decodedFrame.size();
    size_t samplesToCopy = std::min(samplesInFrame, (size_t)samplesToFill);
    std::copy(decodedFrame.begin(), decodedFrame.begin() + samplesToCopy, out);

    // If the decoded frame was smaller than requested, fill the rest with silence
    if (samplesToCopy < (size_t)samplesToFill) {
        std::fill(out + samplesToCopy, out + samplesToFill, 0);
        // std::cout << "[PortAudioPlayback] Jitter: Decoded frame too small, padding with silence." << std::endl;
    }

    // Jitter buffer placeholder: If decodedFrame was larger than needed,
    // you'd typically push the remainder back to a JitterBuffer, not to the main inputQueue.

    // If the playback module is stopping, signal PortAudio to stop the stream
    if (!self->a_IsRunning.load()) {
        std::cout << "[PortAudioPlayback] Stopping callback signal." << std::endl;
        return paComplete;
    }

    return paContinue; // Continue playback
}
