#pragma once

#ifndef __SERDE_H__
#define __SERDE_H__

#include "Physical/Frame.h"
#include "MAC/MACFrame.h"
#include "MAC/ACK.h"

/* Conversion from MAC Frame to Byte array (inplace) */

inline uint8_t *serialize(MACFrame *frame)
{
    return reinterpret_cast<uint8_t *>(frame);
}

inline MACFrame *deserialize(uint8_t *data)
{
    return reinterpret_cast<MACFrame *>(data);
}

inline Frame convertFrame(MACFrame *macframe)
{        
    uint8_t *serialized = serialize(macframe);
    if (macframe->type == Config::ACK)
        return ACK(serialized);
    else if (macframe->type == Config::DATA)
        return Frame(serialized);
    else 
    {
        std::cerr << "convertFrame: Unknown frame type. \n";
        return Frame();
    }
}

inline void convertMACFrame(const Frame &phy, MACFrame *macFrame)
{
    uint8_t *serialized = serialize(macFrame);
    phy.getData(serialized);
}

#endif