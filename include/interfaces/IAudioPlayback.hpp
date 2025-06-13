#ifndef I_AUDIO_PLAYBACK_HPP
#define I_AUDIO_PLAYBACK_HPP

#include "../ThreadSafeQueue.hpp"

#include "opus_types.h"
#include <vector>

using AudioFrame = std::vector<opus_int16>;

struct IAudioPlayback
{
public:
    virtual ~IAudioPlayback() = default; // Virtual destructor for proper polymorphic cleanup
    virtual bool start() = 0;            // Start playing audio
    virtual void stop() = 0;             // Stop playing audio
    virtual void setInputQueue(std::shared_ptr<ThreadSafeQueue<AudioFrame>> queue) = 0; // Set where to pull frames from
    virtual int getSampleRate() const = 0; // Get the sample rate of the playback device
    virtual int getChannels() const = 0;   // Get the number of channels
    virtual int getFrameSize() const = 0;  // Get the frame size (samples per buffer/packet)
};

#endif // I_AUDIO_PLAYBACK_HPP
