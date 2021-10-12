#pragma once

#ifndef __FRAME_H__
#define __FRAME_H__

#include "Ringbuffer.hpp"
#include "Audio.h"
using namespace juce;

/**
 * Frame should be created after the header is set.
 */

class Frame 
{
public:
    Frame(const DataType &data, int start);
    ~Frame();

    const float *getReadPointer() const;

    void addToBuffer(RingBuffer<float> &buffer) const;

    static void generateHeader();
    static void setFrameProperties(int bitLen, int frameLen, int freq);

    static int getBitPerFrame();
    static int getFrameLength();

private:

    void modulate();
    void addHeader();

    AudioType frameAudio;
    
    static int bitLen;
    static int headerLen;
    static int frameLen;
    static int freq;
    static int bitPerFrame;

    static AudioType header;

};

#endif