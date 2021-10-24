#include "MAC/ACK.h"
#include "Physical/Modulator.h"
#include "Utils/IOFunctions.hpp"

ACK::ACK(const uint8_t *pdata)
    : Frame()
{
    frameAudio.setSize(1, Config::ACK_LENGTH);
    DataType ack;
    ack.addArray(pdata, Config::BIT_PER_ACK / 8);
    
    addHeader();
    Modulator::modulate(byteToBit(ack), 0, Config::BIT_PER_ACK, *this);
}
