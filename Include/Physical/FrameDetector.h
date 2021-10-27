#pragma once

#ifndef __DEMODULATOR_H__
#define __DEMODULATOR_H__

#include <JuceHeader.h>
#include "Utils/Ringbuffer.hpp"
#include "mkl.h"

class Frame;

using namespace juce;

typedef Array<uint8_t> DataType;
typedef AudioBuffer<float> AudioType;

class FrameDetector
{
public:
    FrameDetector();
    ~FrameDetector();

    void checkHeader();
    Frame detectAndGet();

    void clear();
    bool isTimeout();
    RingBuffer<float> detectorBuffer;

private:
    std::vector<float> dotproducts;
    std::vector<float> powers;
    std::function<float(int, const float *, const float *)> mkl_dot;
    std::function<float(int, const float *, const float *)> power;

    int headerOffset = 0;
    int prevMaxPos = -1;
    float prevMax = 0.0f;
    int frameCountdown;
    int stopCountdown;
    bool found;

    void resetState();
};

#endif