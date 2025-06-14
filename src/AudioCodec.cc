#include "AudioCodec.hpp"
#include "opus.h"
#include "opus_defines.h"
#include "opus_types.h"
#include <iostream>

AudioCodec::~AudioCodec()
{
    if(m_Encoder) {
        opus_encoder_destroy(m_Encoder);
        m_Encoder = nullptr;
        std::cout << "[AudioCodec] Opus Encoder destroyed.\n";
    }
    if(m_Decoder) {
        opus_decoder_destroy(m_Decoder);
        m_Decoder = nullptr;
        std::cout << "[AudioCodec] Opus Decoder destroyed.\n";
    }
}

bool AudioCodec::initEncoder(int sampleRate, int channels, int application)
{
    if(m_Encoder) {
        std::cerr << "[AudioCodec] Encoder already initialized.\n";
        return false;
    }

    int error;
    int size = opus_encoder_get_size(channels);       // memory needed for encoder state
    m_Encoder = (OpusEncoder*)new char[size];           // allocate memory
    if(!m_Encoder) {
        std::cerr << "[AudioCodec] Failed to allocate memory for Opus Encoder.\n";
        return false;
    }

    // Initialize the encoder
    error = opus_encoder_init(m_Encoder, sampleRate, channels, application);
    if(error != OPUS_OK) {
        std::cerr << "[AudioCodec] Failed to initialize Opus Encoder: " << opus_strerror(error) << std::endl;
        delete[] (char*)m_Encoder;
        m_Encoder = nullptr;
        return false;
    }

    m_SampleRate = sampleRate;
    m_Channels = channels;

    // Configure Encoder
    opus_encoder_ctl(m_Encoder, OPUS_SET_BITRATE(20000));     // Target bitrate 20kbps
    opus_encoder_ctl(m_Encoder, OPUS_SET_VBR(0));             // Disable VBR (Constant Bit Rate)
    opus_encoder_ctl(m_Encoder, OPUS_SET_COMPLEXITY(8));      // Medium complexity

    std::cout << "[AudioCodec] Opus encoder initialized (SR: " << m_SampleRate
        << ", CH: " << m_Channels << ", App: " << application << ")" << std::endl;
        return true;
}

bool AudioCodec::initDecoder(int sampleRate, int channels)
{
    if(m_Decoder) {
        std::cerr << "[AudioCodec] Decoder already initialized." << std::endl;
        return false;
    }

    int error;
    int size = opus_decoder_get_size(channels);     // memory for decoder state
    m_Decoder = (OpusDecoder*)new char[size];       // allocate memory
    if (!m_Decoder) {
        std::cerr << "[AudioCodec] Failed to allocate memory for Opus decoder." << std::endl;
        return false;
    }

    error = opus_decoder_init(m_Decoder, sampleRate, channels);
    if(error != OPUS_OK) {
        std::cerr << "[AudioCodec] Failed to initialize Opus Decoder: " << opus_strerror(error) << std::endl;
        delete[] (char*)m_Decoder;
        m_Decoder = nullptr;
        return false;
    }
    m_Channels = channels;
    m_SampleRate = sampleRate;

    std::cout << "[AudioCodec] Opus decoder initialized (SR: " << m_SampleRate << ", CH: "
    << m_Channels << ")" << std::endl;
    return true;
}

int AudioCodec::encode(const opus_int16* pcm, int frameSize, unsigned char* opusPacket, int maxPacketSize)
{
    if(!m_Encoder) {
        std::cerr << "[AudioCodec] Error: Encoder not initialized." << std::endl;
        return OPUS_BAD_ARG;
    }
    if (!pcm || !opusPacket || maxPacketSize <= 0 || frameSize <= 0) {
        std::cerr << "[AudioCodec] Error: Invalid arguments for encode." << std::endl;
        return OPUS_BAD_ARG;
    }

    // `opus_encode` expects frameSize to be number of samples PER CHANNEL
    int result = opus_encode(m_Encoder, pcm, frameSize, opusPacket, maxPacketSize);
    if (result < 0) {
        std::cerr << "[AudioCodec] Opus encoding failed: " << opus_strerror(result) << std::endl;
    }
    return result; // Returns number of bytes in encoded packet, or error code
}

int AudioCodec::decode(const unsigned char* opusPacket, int packetSize, opus_int16* pcm, int maxFrameSize) {
    if (!m_Decoder) {
        std::cerr << "[AudioCodec] Error: Decoder not initialized." << std::endl;
        return OPUS_BAD_ARG;
    }
    if (!pcm || !opusPacket || packetSize < 0 || maxFrameSize <= 0) {
        std::cerr << "[AudioCodec] Error: Invalid arguments for decode." << std::endl;
        return OPUS_BAD_ARG;
    }

    // `opus_decode` expects maxFrameSize to be number of samples PER CHANNEL
    int result = opus_decode(m_Decoder, opusPacket, packetSize, pcm, maxFrameSize, 0); // 0 for FEC, not needed for now

    if (result < 0) {
        std::cerr << "[AudioCodec] Opus decoding failed: " << opus_strerror(result) << std::endl;
    }
    return result; // Returns number of samples decoded PER CHANNEL, or error code
}
