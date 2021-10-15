#pragma once

#ifndef __FRAME_H__
#define __FRAME_H__

#include <JuceHeader.h>
#include <vector>
#include "Ringbuffer.hpp"

#define BAND_WIDTH 2
#define PI acos(-1)

using namespace juce;

typedef Array<int8_t> DataType;
typedef AudioBuffer<float> AudioType;

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

    static void frameInit();

    static void setFrameProperties(int bitLen, int frameLen);
    static void Frame::demodulate(const float *samples, DataType &out);

    static int getBitPerFrame();
    static int getFrameLength();
    static int getHeaderLength();
    static int getBitLength();

    static const float *getHeader();
    constexpr static int sampleRate = 48000;

private:

    void modulate(const DataType &data, int start);
    void addHeader();
    void addSound(const AudioType &src);

    static AudioType generateSound(int freq, int length, float initPhase);
    static void generateHeader();

    AudioType frameAudio;
    int audioPos = 0;
    
    static int bitLen;
    static int headerLen;
    static int frameLen;
    static int bitPerFrame;

    static AudioType header;
    static std::vector<AudioType> modulateSound;

};

#endif