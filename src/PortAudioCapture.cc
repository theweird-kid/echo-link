#include "PortAudioCapture.hpp"
#include <iostream>
#include <portaudio.h>

PortAudioCapture::PortAudioCapture(int sampleRate, int channels, int frameSize)
    : m_SampleRate(sampleRate), m_Channels(channels), m_FrameSize(frameSize)
{
    a_IsRunning.store(false);
    // Initialize PortAudio in the global scope, to avoid multiple initializations
}

PortAudioCapture::~PortAudioCapture()
{
    // Cleanup PortAudio resources
    stop();
}

void PortAudioCapture::setOutputQueue(std::shared_ptr<ThreadSafeQueue<AudioFrame>> queue)
{
    m_OutputQueue = std::move(queue);
}

bool PortAudioCapture::start()
{
    if(m_OutputQueue == nullptr) {
        std::cerr << "[PortAudioCapture] Output queue not set. cannot start capture";
        return false;
    }
    if(a_IsRunning.load()) {
        std::cout << "[PortAudioCapture] Already running.\n";
        return true;
    }

    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    if(inputParameters.device == paNoDevice) {
        std::cerr << "[PortAudioCapture] No default input device found.\n";
        return false;
    }
    inputParameters.channelCount = m_Channels;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &m_InputStream,       // Pointer to stream handle
        &inputParameters,    // Input stream parameters
        NULL,                // No output stream
        m_SampleRate,         // Sample rate
        m_FrameSize,          // Frames per buffer (size of chunks for callback)
        paNoFlag,            // No special flags
        paInputCallback,     // The static callback function
        this                 // User data to pass to the callback (pointer to this instance)
    );
    if(err != paNoError) {
        std::cerr << "[PortAudioCapture] Failed to open input stream: " << Pa_GetErrorText(err) << "\n";
        m_InputStream = nullptr;
        return false;
    }

    err = Pa_StartStream(m_InputStream);
    if(err != paNoError) {
        std::cerr << "[PortAudioCapture] Failed to start input stream: " << Pa_GetErrorText(err) << "\n";
        Pa_CloseStream(m_InputStream);
        m_InputStream = nullptr;
        return false;
    }

    a_IsRunning.store(true);
    std::cout << "[PortAudioCapture] Started audio capture (SR: " << m_SampleRate
        << ", CH: " << m_Channels << ", Frame: " << m_FrameSize << ")" << std::endl;
    return true;
}

void PortAudioCapture::stop() {
    if (a_IsRunning.load())
    { // Check if running before attempting to stop
        a_IsRunning.store(false); // Signal callback to stop processing (though Pa_StopStream also does this)
        if(m_InputStream != nullptr) {
            PaError err = Pa_StopStream(m_InputStream); // Stop the stream
            if (err != paNoError) {
                std::cerr << "[PortAudioCapture] Error stopping input stream: " << Pa_GetErrorText(err) << std::endl;
            }
            err = Pa_CloseStream(m_InputStream); // Close the stream
            if (err != paNoError) {
                std::cerr << "[PortAudioCapture] Error closing input stream: " << Pa_GetErrorText(err) << std::endl;
            }
            m_InputStream = nullptr; // Clear stream handle
        }
        std::cout << "[PortAudioCapture] Audio capture stopped." << std::endl;
    }
}

int PortAudioCapture::paInputCallback(const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    // Cast userData back to our PortAudioCapture instance
    PortAudioCapture* self = static_cast<PortAudioCapture*>(userData);

    // If inputBuffer is NULL or stream is stopping, return accordingly
    if(inputBuffer == NULL || !self->a_IsRunning.load()) {
        std::cout << "[PortAudioCapture] Callback: Input buffer NULL or stopping. Returning paComplete." << std::endl;
        return paComplete; // Signal PortAudio to stop the stream
    }

    const opus_int16* pcmData = static_cast<const opus_int16*>(inputBuffer);
    AudioFrame capturedFrame(framesPerBuffer * self->m_Channels); // Create a new AudioFrame

    // Copy captured PCM data from the input buffer into our AudioFrame vector
    std::copy(pcmData, pcmData + capturedFrame.size(), capturedFrame.begin());

    // Push the captured frame to the output queue
    if(self->m_OutputQueue) {
        if(self->m_OutputQueue->is_shutting_down()) {
                std::cout << "[PortAudioCapture] Callback: Output queue shutting down. Returning paComplete." << std::endl;
                return paComplete;
        }
        self->m_OutputQueue->push(capturedFrame);
    } else {
        std::cerr << "[PortAudioCapture] Callback: Output queue is nullptr! Captured data dropped." << std::endl;
    }

    return paContinue; // Continue capturing
}
