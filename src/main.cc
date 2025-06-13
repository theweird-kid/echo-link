#include "FakeAudioSource.hpp"
#include "PortAudioPlayback.hpp"
#include "ThreadSafeQueue.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

int main() {
    // Parameters (must match your audio file and hardware)
    std::string audioFilePath = "../data/audio.raw"; // Must be 16-bit PCM, interleaved if stereo
    int sampleRate = 48000;
    int channels = 2;
    int frameSize = 960;

    // Shared queue for audio frames
    auto queue = std::make_shared<ThreadSafeQueue<AudioFrame>>();

    // Instantiate source and playback
    FakeAudioSource source(audioFilePath, sampleRate, channels, frameSize);
    PortAudioPlayback playback(sampleRate, channels, frameSize);

    // Connect the queue
    source.setOutputQueue(queue);
    playback.setInputQueue(queue);

    // Start playback first (so it's ready to consume)
    if (!playback.start()) {
        std::cerr << "Failed to start PortAudioPlayback" << std::endl;
        return 1;
    }

    // Start the fake audio source
    if (!source.start()) {
        std::cerr << "Failed to start FakeAudioSource" << std::endl;
        playback.stop();
        return 1;
    }

    // Let it play for a while
    std::this_thread::sleep_for(std::chrono::seconds(50));

    // Stop both
    source.stop();
    playback.stop();

    std::cout << "Test complete." << std::endl;
    return 0;
}
