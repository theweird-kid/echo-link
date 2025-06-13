#ifndef I_AUDIO_SOURCE_HPP
#define I_AUDIO_SOURCE_HPP

#include <opus/opus.h>
#include <vector>
#include "./thread_safe_queue.hpp" // Assuming this is in ThreadSafeQueue.h

// Define the type of audio frame we'll be pushing
// For raw PCM, opus_int16 (short) is common.
// Let's assume 16-bit stereo for now.
using AudioFrame = std::vector<opus_int16>;

class IAudioSource {
public:
    virtual ~IAudioSource() = default; // Virtual destructor for proper polymorphic cleanup
    virtual bool start() = 0;          // Start capturing/reading audio
    virtual void stop() = 0;           // Stop capturing/reading audio
    virtual void setOutputQueue(std::shared_ptr<ThreadSafeQueue<AudioFrame>> queue) = 0; // Set where to push frames
    virtual int getSampleRate() const = 0; // Get the sample rate of the source
    virtual int getChannels() const = 0;   // Get the number of channels
    virtual int getFrameSize() const = 0;  // Get the frame size (samples per buffer/packet)
};

#endif // I_AUDIO_SOURCE_HPP
