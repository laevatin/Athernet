#pragma once

#ifndef __FRAME_H__
#define __FRAME_H__

#include <JuceHeader.h>
#include <vector>
#include "Utils/Ringbuffer.hpp"
#define PI acos(-1)

using namespace juce;

typedef Array<uint8_t> DataType;
typedef AudioBuffer<float> AudioType;

class MACHeader;

/**
 * Data structure for the frames
 */

class Frame 
{
public:
    Frame();

    /* Encode the frame and modulate. */
    Frame(const uint8_t *pdata);

    /* Demodulate and decode the frame. 
     `audio` should contain at least (Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH) * Config::BIT_LENGTH samples */
    Frame(MACHeader *macHeader, const float *audio);

    ~Frame() = default;

    /* Move constructor */
    Frame(Frame &&other);

    /* Copy constructor */
    Frame(const Frame &other);

    void addToBuffer(RingBuffer<float> &buffer) const;
    void addSound(const AudioType &src);
    void getData(DataType &out) const;
    void getData(uint8_t *out) const;

    const bool isGoodFrame() const;
    const bool isACK() const;
    const uint8_t dataLength() const;
    const uint8_t id() const;

protected:
    void addHeader();

    AudioType frameAudio;

    /* frameData is byte array */
    DataType frameData; 
    bool m_isACK = false;
    bool m_isGood = false;

    uint8_t m_dataLength = 0;
    uint8_t m_id;
    int m_audioPos = 0;
};

#endif