#include "MAC/ACK.h"
#include "Physical/Modulator.h"
#include "Utils/IOFunctions.hpp"
#include "Config.h"

ACK::ACK(const uint8_t *pdata, uint8_t id)
    : Frame()
{
    frameAudio.setSize(1, Config::ACK_LENGTH);
    DataType ack;
    ack.addArray(pdata, Config::BIT_PER_ACK / 8);
    m_isACK = true;
    m_id = id;
    
    addHeader();
    Modulator::modulate(byteToBit(ack), 0, Config::BIT_PER_ACK, *this);
}

ACK::ACK(MACHeader *header)
    : Frame()
{
    auto *header_uint8 = reinterpret_cast<uint8_t *>(header);
    m_isACK = true;
    m_id = header->id;
    
    frameData.addArray(header_uint8, Config::MACHEADER_LENGTH / 8);
}

ACK::ACK(ACK&& other) noexcept
    : Frame(std::move(other))
{}
