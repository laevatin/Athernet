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

    /* Encode the frame and modulate. */
    Frame(const DataType &data, int start);

    /* Demodulate and decode the frame. 
     `audio` should contain at least SAMPLE_PER_FRAME samples */
    Frame(const float *audio);

    ~Frame();

    void addToBuffer(RingBuffer<float> &buffer) const;
    void addSound(const AudioType &src);
    void getData(DataType &out);

private:

    void addHeader();

    AudioType frameAudio;
    DataType frameData; 

    int audioPos = 0;
};

#endif