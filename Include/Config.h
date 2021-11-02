#pragma once

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <JuceHeader.h>
#include <vector>
#include <chrono>
#include "Physical/Audio.h"

typedef juce::Array<uint8_t> DataType;
typedef juce::AudioBuffer<float> AudioType;

using namespace std::chrono_literals;

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

    constexpr static int MACHEADER_LENGTH = sizeof(MACHeader) * 8;
    constexpr static int MACDATA_PER_FRAME = DATA_PER_FRAME / 8 - MACHEADER_LENGTH / 8;

    constexpr static int BIT_PER_ACK  = 40;

    constexpr static int BAND_WIDTH   = 2;

    constexpr static int SAMPLE_PER_FRAME = BIT_PER_FRAME * BIT_LENGTH / BAND_WIDTH;
    constexpr static int FRAME_LENGTH     = SAMPLE_PER_FRAME + HEADER_LENGTH + 10;
    constexpr static int ACK_LENGTH       = HEADER_LENGTH + BIT_LENGTH * BIT_PER_ACK;

    constexpr static uint8_t ACK = 0xAC;
    constexpr static uint8_t DATA = 0xDA;

    constexpr static uint8_t SENDER = 0xED;
    constexpr static uint8_t RECEIVER = 0xCE;

    constexpr static auto ACK_TIMEOUT = 100ms;

    constexpr static int RECV_TIMEOUT = 1;

    constexpr static int PENDING_QUEUE_SIZE = 20;

    constexpr static enum AudioIO::state STATE = AudioIO::state::BOTH;

    static AudioType header;
    static std::vector<AudioType> modulateSound;

private:
    void initPreamble();

    void initAudio();

    AudioType generateSound(int freq, int length, float initPhase);

};


#endif
