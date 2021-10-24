#pragma once

#ifndef __MACFRAME_H__
#define __MACFRAME_H__

#include <JuceHeader.h>
#include "Config.h"

struct MACFrame
{
    uint8_t dest;
    uint8_t src;
    uint8_t type;
    uint8_t len;
    uint8_t id;

    uint8_t data[Config::DATA_PER_FRAME / 8 - 5];
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

    /* Resource management */
    void destoryFrame(MACFrame *frame);

private:
    constexpr static uint8_t ACK = 0xAC;
    constexpr static uint8_t DATA = 0xDA;

    constexpr static uint8_t SENDER = 0xED;
    constexpr static uint8_t RECEIVER = 0xCE;

    uint8_t nextid;
};

#endif
