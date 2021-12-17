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

/**
 * Data structure for the frames
 */
struct FrameHeader
{
    uint16_t length;
};

class Frame 
{
public:
    Frame() = default;

    /* Encode the frame and modulate. */
    Frame(const void *phys_data, uint16_t length);

    /* Demodulate and decode the frame. 
     `audio` should contain at least (Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH) * Config::BIT_LENGTH samples */
    Frame(FrameHeader *header, const float *audio);

    ~Frame() = default;

    /* Move constructor */
    Frame(Frame &&other) noexcept;

    /* Copy constructor */
    Frame(const Frame &other) = default;

    void getData(DataType &out) const;

    bool isGoodFrame() const;
    uint16_t dataLength() const;

protected:

    FrameHeader m_header;
    /* frameData is byte array */
    DataType m_frameData;
    bool m_isGood = false;

    friend class AudioFrame;
};

#endif