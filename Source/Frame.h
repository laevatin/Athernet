#pragma once

#ifndef __FRAME_H__
#define __FRAME_H__

#include <JuceHeader.h>
#include <vector>
#include "Ringbuffer.hpp"

#define PI acos(-1)

using namespace juce;

typedef Array<int8_t> DataType;
typedef AudioBuffer<float> AudioType;

/**
 * Data structure for the frames
 */

class Frame 
{
public:
    Frame(const DataType &data, int start);
    ~Frame();

    void addToBuffer(RingBuffer<float> &buffer) const;

    static void Frame::demodulate(const float *samples, DataType &out);

private:

    void modulate(const DataType &data, int start);
    void addHeader();
    void addSound(const AudioType &src);

    AudioType frameAudio;
    int audioPos = 0;
};

#endif