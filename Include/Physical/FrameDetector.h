#pragma once

#ifndef __DEMODULATOR_H__
#define __DEMODULATOR_H__

#include <JuceHeader.h>
#include "Utils/Ringbuffer.hpp"
#include "MAC/Serde.h"
#include "Physical/Frame.h"

using namespace juce;

typedef Array<uint8_t> DataType;
typedef AudioBuffer<float> AudioType;

class FrameDetector
{
public:
    FrameDetector();
    ~FrameDetector() = default;

    void detectAndGet(RingBuffer<float> &detectorBuffer, std::list<Frame> &received);

    void clear();

private:

    enum state {
        CK_HEADER,
        FD_HEADER,
        GET_DATA
    } m_state;

    int headerOffset = 0;
    int prevMaxPos = -1;
    float prevMax = 0.0f;
    DataType frameHeader;

    void resetState();
    void checkHeader(RingBuffer<float> &detectorBuffer);
    static DataType getMACHeader(const float *samples);

};

#endif