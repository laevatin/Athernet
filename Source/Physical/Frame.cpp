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
    m_header.length = length;
    if (length == Config::DATA_PER_FRAME)
        m_header.length = Config::BIT_PER_FRAME / 8;

    m_frameData.addArray(reinterpret_cast<uint8_t *>(&m_header), sizeof(struct FrameHeader));
    m_frameData.addArray(static_cast<const uint8_t *>(phys_data), length);

    /* Add FEC when length == Config::DATA_PER_FRAME */
    if (length == Config::DATA_PER_FRAME)
        m_frameData = AudioDevice::codec.encodeBlock(m_frameData, 0);
}
// TODO: DEBUG with Config::BIT_PER_FRAME because of adding the physical header
Frame::Frame(FrameHeader *header, const float *audio)
{
    DataType bitArray, byteArray;
    for (int i = 0; i < header->length * 8 * Config::BIT_LENGTH / Config::BAND_WIDTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(audio + i, bitArray);

    byteArray.addArray(reinterpret_cast<uint8_t *>(header), sizeof(struct FrameHeader));
    byteArray.addArray(bitToByte(bitArray));

    if (header->length == Config::BIT_PER_FRAME / 8)
        m_isGood = AudioDevice::codec.decodeBlock(byteArray, m_frameData, 0);
    else
    {
        m_frameData = std::move(byteArray);
        m_isGood = true;
    }
    this->m_header = *header;
}

Frame::Frame(Frame &&other) noexcept
    : m_header(other.m_header),
    m_frameData(std::move(other.m_frameData)),
    m_isGood(std::exchange(other.m_isGood, false))
{}

void Frame::getData(DataType &out) const
{
    out.addArray(m_frameData, sizeof(struct FrameHeader), m_frameData.size());
}

bool Frame::isGoodFrame() const
{
    return m_isGood;
}

uint16_t Frame::dataLength() const
{
    if (m_isGood)
        return m_header.length;
    return 0;
}
