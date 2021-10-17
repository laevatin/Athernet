#pragma once

#ifndef __FRAME_H__
#define __FRAME_H__

#include <JuceHeader.h>
#include <vector>
#include "Ringbuffer.hpp"

#define PI acos(-1)

using namespace juce;

typedef Array<uint8_t> DataType;
typedef AudioBuffer<float> AudioType;

/**
 * Data structure for the frames
 */

class Frame 
{
public:
    Frame();
    Frame(const DataType &data, int start);
    ~Frame();

    void addToBuffer(RingBuffer<float> &buffer) const;
    void addSound(const AudioType &src);

private:

    void modulate(const DataType &data, int start);
    void addHeader();

    AudioType frameAudio;
    int audioPos = 0;
};

#endif