#include "MAC/ACK.h"
#include "Physical/Modulator.h"
#include "Utils/IOFunctions.hpp"

ACK::ACK(const uint8_t *pdata)
    : Frame()
{
    frameAudio.setSize(1, Config::ACK_LENGTH);
    DataType ack;
    ack.addArray(pdata, Config::BIT_PER_ACK / 8);
    isACK = true;
    
    addHeader();
    Modulator::modulate(byteToBit(ack), 0, Config::BIT_PER_ACK, *this);
}

ACK::ACK(MACHeader *header)
    : Frame()
{
    uint8_t *header_uint8 = reinterpret_cast<uint8_t *>(header);
    isACK = true;
    
    frameData.addArray(header_uint8, Config::MACHEADER_LENGTH / 8);
}
