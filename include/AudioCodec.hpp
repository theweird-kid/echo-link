#ifndef AUDIO_CODEC_HPP
#define AUDIO_CODEC_HPP

#include <opus.h>
#include <opus_custom.h>

class AudioCodec
{
private:
    OpusEncoder* m_Encoder = nullptr;
    OpusDecoder* m_Decoder = nullptr;

    int m_SampleRate{0};
    int m_Channels{0};

public:
    AudioCodec() = default;
    ~AudioCodec();

    // returns true on success
    bool initEncoder(int sampleRate, int channels, int application);
    // returns true on success
    bool initDecoder(int sampleRate, int channels);

    // Encodes a raw PCM audio frame into an Opus packet.
    // pcm: Pointer to the raw PCM data (opus_int16 array).
    // frameSize: Number of samples per channel in the PCM frame.
    // opusPacket: Buffer to store the encoded Opus packet.
    // maxPacketSize: Maximum size of the opusPacket buffer in bytes.
    // Returns the number of bytes encoded, or a negative Opus error code on failure.
    int encode(const opus_int16* pcm, int frameSize, unsigned char* opusPacket, int maxPacketSize);

    // Decodes an Opus packet back into raw PCM audio
    // opusPacket: Pointer to the opus encoded packet.
    // packetSize: Size of the Opus packet in bytes.
    // pcm: Buffer to store the decoded PCM data (opus_int16 array)
    // maxFrameSize: Maximum number of samples per channel that the PCM buffer can hold.
    // Returns the number of samples decoded per channel, or a negative Opus error code on failure.
    int decode(const unsigned char* opusPacket, int packetSize, opus_int16* pcm, int maxFrameSize);

    int getSampleRate() const { return m_SampleRate; }
    int getChannels() const { return m_Channels; }
};

#endif // AUDIO_CODEC_HPP
