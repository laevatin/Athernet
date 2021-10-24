#pragma once

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <JuceHeader.h>
#include <vector>
#include "Physical/Audio.h"

typedef juce::Array<uint8_t> DataType;
typedef juce::AudioBuffer<float> AudioType;

/* Configuration of the physical layer. */
class Config {

public:

    Config() 
    {
        initPreamble();
        initAudio();
    }

    constexpr static int SAMPLE_RATE   = 48000;

    constexpr static int HEADER_LENGTH = 440;

    constexpr static int BIT_LENGTH    = 32;

    constexpr static int DATA_PER_FRAME = 8 * 62;
    constexpr static int BIT_PER_FRAME  = 8 * 72;

    constexpr static int BAND_WIDTH   = 2;

    constexpr static int SAMPLE_PER_FRAME = BIT_PER_FRAME * BIT_LENGTH / BAND_WIDTH;
    constexpr static int FRAME_LENGTH     = SAMPLE_PER_FRAME + HEADER_LENGTH + 10;

    constexpr static int RECV_TIMEOUT = 1;

    constexpr static int PENDING_QUEUE_SIZE = 20;

    constexpr static enum AudioDevice::state STATE = AudioDevice::state::BOTH;

    static AudioType header;
    static std::vector<AudioType> modulateSound;

private:
    void initPreamble();

    void initAudio();

    AudioType generateSound(int freq, int length, float initPhase);

};


#endif
