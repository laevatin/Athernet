#pragma once

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <JuceHeader.h>
#include <vector>
#include <chrono>

typedef juce::AudioBuffer<float> AudioType;

using namespace std::chrono_literals;

class MACHeader;

#define VERBOSE_MAC
//#define SLIDING_WINDOW_DEBUG

enum state {
    SENDING = 1,
    RECEIVING = 2,
    BOTH = 3
};

/* Configuration of the physical layer. */
class Config {

public:

    Config() {
        initPreamble();
        initAudio();
    }

    constexpr static int SAMPLE_RATE = 48000;

    constexpr static int HEADER_LENGTH = 80;

    constexpr static int BIT_LENGTH = 6;

    constexpr static int IP_PACKET_PAYLOAD = 94; // MACDATA_PER_FRAME / 8 - sizeof(ANetIP) - sizeof(ANetUDP)

    constexpr static int DATA_PER_FRAME = 8 * 120;
    constexpr static int BIT_PER_FRAME = 8 * 127;

    constexpr static int PHYHEADER_LENGTH = 2 * 8; // sizeof(FrameHeader) * 8
    constexpr static int MACHEADER_LENGTH = 8 * 8; // sizeof(MACHeader) * 8
    constexpr static int MACDATA_PER_FRAME = DATA_PER_FRAME / 8 - MACHEADER_LENGTH / 8;

    constexpr static int BAND_WIDTH = 2;

    constexpr static uint8_t SELF = 0xCE;
    constexpr static uint8_t OTHER = 0xEC;

    constexpr static auto ACK_TIMEOUT = 400ms;

    constexpr static int SLIDING_WINDOW_SIZE = 3;

    constexpr static int POWER_AVG_LEN = 100;
    constexpr static float POWER_THOR = 100.0f;

    constexpr static auto BACKOFF_TSLOT = 30ms; // CSMA Tslot in milliseconds

    constexpr static enum state STATE = BOTH;
    constexpr static char IP_ATHERNET[16] = "192.168.1.2"; // Node1
    constexpr static char PORT_ATHERNET[] = "4567";

    static char IP_ETHERNET[16];

    static AudioType header;
    static std::vector<AudioType> modulateSound;

private:
    static void initPreamble();

    void initAudio();

    static AudioType generateSound(int freq, int length, float initPhase);

};


#endif
