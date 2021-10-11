#pragma once

#ifndef __FRAME_H__
#define __FRAME_H__

#include "Ringbuffer.hpp"
#include <JuceHeader.h>
using namespace juce;

/**
 * Frame should be created after the header is set.
 */

class Frame 
{
public:
    Frame(const Array<int8_t> &data, int start, int len);
    ~Frame();

    const float *getReadPointer() const;

    void addToBuffer(RingBuffer<float> &buffer) const;

    static void setHeader(const AudioBuffer<float> &header);
    static void setFrameProperties(int bitLen, int headerLen, int frameLen, int freq);

private:

    void modulate();
    void addHeader();

    AudioBuffer<float> frameAudio;
    
    static int bitLen;
    static int headerLen;
    static int frameLen;
    static int freq;

    static AudioBuffer<float> header;

};

#endif