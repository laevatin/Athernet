#pragma once

#ifndef __DEMODULATOR_H__
#define __DEMODULATOR_H__

#include <JuceHeader.h>
#include "Demodulator.h"
#include "Accumulator.h"
#include "mkl.h"

using namespace juce;

typedef Array<int8_t> DataType;
typedef AudioBuffer<float> AudioType;

class Demodulator 
{
public:
    Demodulator();
    ~Demodulator();

    void checkHeader(RingBuffer<float> &receiver);
    void demodulate(const float *samples, DataType &dataOut);
    bool isGettingBit();

private:
    Accumulator<float> power;

    int frameCountdown;
    int headerPos = -1;
    bool status;
};

#endif