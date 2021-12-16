#include "Physical/Frame.h"
#include "Physical/Codec.h"
#include "Physical/Modulator.h"
#include "MAC/MACFrame.h"
#include "MAC/MACManager.h"
#include "Utils/IOFunctions.hpp"
#include <fstream>
#include "Config.h"

std::ofstream debug_file;

Frame::Frame(const void *phys_data, uint16_t length)
    : m_isGood(true)
{
    jassert(length <= Config::DATA_PER_FRAME / 8);
    header.length = length;
    if (length == Config::DATA_PER_FRAME)
        header.length = Config::BIT_PER_FRAME / 8;

    m_frameData.addArray((void *)&header, sizeof(struct FrameHeader));
    m_frameData.addArray(phys_data, length);

    /* Add FEC when length == Config::DATA_PER_FRAME */
    if (length == Config::DATA_PER_FRAME)
        m_frameData = AudioDevice::codec.encodeBlock(m_frameData, 0);
}

Frame::Frame(FrameHeader *header, const float *audio)
{
    DataType bitArray, byteArray;
    for (int i = 0; i < header->length * 8 * Config::BIT_LENGTH / Config::BAND_WIDTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(audio + i, bitArray);

    byteArray.addArray((void *)header, sizeof(struct FrameHeader));
    byteArray.addArray(bitToByte(bitArray), header->length);

    if (header->length == Config::BIT_PER_FRAME / 8)
    {
        m_isGood = AudioDevice::codec.decodeBlock(byteArray, m_frameData, 0);
    }
    else
    {
        m_frameData = std::move(byteArray);
        m_isGood = true;
    }
    this->header = *header;
}

Frame::Frame(Frame &&other) noexcept
    : m_frameData(std::move(other.m_frameData)),
    m_isGood(std::exchange(other.m_isGood, false))
{}

void Frame::getData(DataType &out) const
{
    out.addArray(m_frameData, sizeof(struct FrameHeader));
}

bool Frame::isGoodFrame() const
{
    return m_isGood;
}

uint16_t Frame::dataLength() const
{
    if (m_isGood)
        return header.length;
    return 0;
}
