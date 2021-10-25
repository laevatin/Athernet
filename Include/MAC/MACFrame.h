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

    /* Create an empty frame */
    MACFrame *createEmpty();

    /* Resource management */
    void destoryFrame(MACFrame *frame);

private:
    uint8_t nextid;
};

#endif
