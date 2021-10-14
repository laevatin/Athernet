#pragma once

#ifndef __DEMODULATOR_H__
#define __DEMODULATOR_H__

#include <JuceHeader.h>
#include "Accumulator.h"
#include "Ringbuffer.hpp"
#include "mkl.h"

using namespace juce;

typedef Array<int8_t> DataType;
typedef AudioBuffer<float> AudioType;

class Demodulator 
{
public:
    Demodulator();
    ~Demodulator();

    void checkHeader();
    void demodulate(DataType &dataOut);
    bool isTimeout();
    bool isGettingBit();
    RingBuffer<float> demodulatorBuffer;

private:
    std::vector<float> dotproducts;
    std::function<float(int, const float *, const float *)> mkl_dot;

    float *bitBuffer;
    int headerOffset = 0;
    int prevMaxPos = -1;
    float prevMax = 0.0f;
    int frameCountdown = 0;
    int stopCountdown;
    bool status;

    void resetState();
};

#endif