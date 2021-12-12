#pragma once

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <JuceHeader.h>
#include <vector>
#include <chrono>

typedef juce::Array<uint8_t> DataType;
typedef juce::AudioBuffer<float> AudioType;

using namespace std::chrono_literals;
struct MACHeader;

enum state {
    SENDING = 1,
    RECEIVING = 2,
    BOTH = 3
};

/* Configuration of the physical layer. */
class Config {

public:

    Config() 
    {
        initPreamble();
        initAudio();
    }

    constexpr static int SAMPLE_RATE   = 48000;

    constexpr static int HEADER_LENGTH = 160;

    constexpr static int BIT_LENGTH    = 6;

    constexpr static int PACKET_PAYLOAD = 94; // MACDATA_PER_FRAME / 8 - sizeof(ANetIP) - sizeof(ANetUDP)

    constexpr static int DATA_PER_FRAME = 8 * 120;
    constexpr static int BIT_PER_FRAME  = 8 * 127;

    constexpr static int MACHEADER_LENGTH = 8 * 8; // sizeof(MACHeader) * 8
    constexpr static int MACDATA_PER_FRAME = DATA_PER_FRAME / 8 - MACHEADER_LENGTH / 8;

    constexpr static int BIT_PER_ACK  = MACHEADER_LENGTH;

    constexpr static int BAND_WIDTH   = 2;

    constexpr static int SAMPLE_PER_FRAME = BIT_PER_FRAME * BIT_LENGTH / BAND_WIDTH;
    constexpr static int FRAME_LENGTH     = SAMPLE_PER_FRAME + HEADER_LENGTH + 10;
    constexpr static int ACK_LENGTH       = HEADER_LENGTH + BIT_LENGTH * BIT_PER_ACK;

    constexpr static uint8_t ACK = 0xAC;
    constexpr static uint8_t DATA = 0xDA;

	constexpr static uint8_t MACPING_REQ = 0xBE;
	constexpr static uint8_t MACPING_REPLY = 0xEF;
    constexpr static uint8_t MACPING_ID = 0xFE;

    constexpr static uint8_t SELF = 0xCE;
    constexpr static uint8_t OTHER = 0xEC;

    constexpr static auto ACK_TIMEOUT = 200ms;

    constexpr static int RECV_TIMEOUT = 1;
    
    constexpr static int POWER_AVG_LEN = 100; 
    constexpr static float POWER_THOR  = 0.01f;

    constexpr static int BACKOFF_TSLOT = 100; // CSMA Tslot in milliseconds

    constexpr static int PENDING_QUEUE_SIZE = 10;

    constexpr static enum state STATE = BOTH;
    constexpr static bool IS_GATEWAY = false;
    constexpr static char* NODE1 = "192.168.1.2";
    constexpr static char* NODE2 = "192.168.1.1";

    static AudioType header;
    static std::vector<AudioType> modulateSound;

private:
    void initPreamble();

    void initAudio();

    AudioType generateSound(int freq, int length, float initPhase);

};


#endif
