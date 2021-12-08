#pragma once

#ifndef __MACFRAME_H__
#define __MACFRAME_H__

#include <JuceHeader.h>
#include "Config.h"
#include "Utils/CRC.h"

typedef juce::Array<uint8_t> DataType;
class Config;

struct MACHeader
{
    uint8_t dest;
    uint8_t src;
    uint8_t type;
    uint8_t len;
    uint8_t id;
    uint16_t crc16;
};

struct MACFrame
{
    struct MACHeader header;
    uint8_t data[Config::MACDATA_PER_FRAME];
};

class MACFrameFactory
{
public:
    MACFrameFactory();
    ~MACFrameFactory();

    /* Create a data frame */
    MACFrame *createDataFrame(const DataType &data, int start, int len);
    
    /* Create an ack frame */
    MACFrame *createACKFrame(uint8_t id);

	/* Create a macping reply*/
	MACFrame* createPingReply(uint8_t id);

	/* Create a macping request*/
	MACFrame* createPingReq(uint8_t id);

    /* Create an empty frame */
    MACFrame *createEmpty();

    /* Resource management */
    void destoryFrame(MACFrame *frame);

    bool checkCRC(MACFrame *frame);

private:
    uint8_t m_nextid;
    CRC::Table<std::uint16_t, 16> m_crcTable;
};

#endif
