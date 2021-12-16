#pragma once

#ifndef __MACFRAME_H__
#define __MACFRAME_H__

#include <JuceHeader.h>
#include "Config.h"
#include "Utils/CRC.h"
#include "Physical/Frame.h"

typedef juce::Array<uint8_t> DataType;

enum class MACType
{
    DATA,
    ACK,
    PING_REQ,
    PING_REP
};

struct MACHeader
{
    uint8_t dest;
    uint8_t src;
    uint8_t type;
    uint8_t id;
    uint16_t len;
    uint16_t crc16;
};

class MACFrame
{
public:
    MACFrame(const void *mac_payload, uint16_t length);
    MACFrame(uint8_t id, MACType type);
    explicit MACFrame(const Frame& frame);

    MACFrame(const MACFrame& other) = default;
    MACFrame(MACFrame&& other) noexcept ;

    uint16_t serialize(void *out) const;
    bool isGood() const;
    const MACHeader& getHeader() const;

private:
    bool checkCRC();

    MACHeader m_macHeader;
    DataType m_payload;
    bool m_isGoodMACFrame;
    static uint8_t nextID;
    static CRC::Table<std::uint16_t, 16> crcTable;
};

#endif
