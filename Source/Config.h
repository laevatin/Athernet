#pragma once

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <JuceHeader.h>
#include <vector>
#include "Audio.h"

typedef juce::Array<int8_t> DataType;
typedef juce::AudioBuffer<float> AudioType;

/* Configuration of the physical layer. */
class Config {

public:

    Config() 
    {
        initPreamble();
        initAudio();
    }

    constexpr static int SAMPLE_RATE = 48000;

    constexpr static int HEADER_LENGTH = 440;

    constexpr static int BIT_LENGTH = 100;
    constexpr static int FRAME_LENGTH = 5490;
    constexpr static int BIT_PER_FRAME = 100;

    constexpr static int BAND_WIDTH = 2;

    constexpr static int RECV_TIMEOUT = 1;

    constexpr static int PENDING_QUEUE_SIZE = 10;

    constexpr static enum AudioDevice::state STATE = AudioDevice::state::BOTH;

    static AudioType header;
    static std::vector<AudioType> modulateSound;

private:
    void initPreamble();

    void initAudio();

    AudioType generateSound(int freq, int length, float initPhase);

};


#endif
