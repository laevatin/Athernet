#pragma once

#ifndef __DEMODULATOR_H__
#define __DEMODULATOR_H__

#include "Audio.h"
#include "Frame.h"
#include "Accumulator.h"
#include "mkl.h"

#define HEADER_BUFFER 2000

typedef Array<int8_t> DataType;
typedef AudioBuffer<float> AudioType;

class Demodulator 
{
public:
    Demodulator();
    ~Demodulator();

    bool checkHeader();

private:
    Accumulator<float> power;

    AudioType headerBuffer;
    int headerPos = -1;

    friend class AudioDevice;
};

#endif