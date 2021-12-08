#include "Physical/Frame.h"
#include "Physical/Codec.h"
#include "Physical/Modulator.h"
#include "MAC/MACFrame.h"
#include "MAC/MACManager.h"
#include "Utils/IOFunctions.hpp"
#include "mkl.h"
#include <fstream>
#include "Config.h"

std::ofstream debug_file;

Frame::Frame(const uint8_t *pdata, uint8_t id)
    : m_isGood(true)
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);
    DataType data;
    data.addArray(pdata, Config::DATA_PER_FRAME / 8);
    m_id = id;
    addHeader();
    DataType encoded = byteToBit(AudioDevice::codec.encodeBlock(data, 0));

    Modulator::modulate(encoded, 0, Config::BIT_PER_FRAME, *this);
}

Frame::Frame()
{}

Frame::Frame(MACHeader *macHeader, const float *audio)
{
    DataType out, tmp;
    uint8_t *macHeader_uint8 = reinterpret_cast<uint8_t *>(macHeader);

    out.addArray(macHeader_uint8, Config::MACHEADER_LENGTH / 8);
    m_dataLength = macHeader->len;
    m_id = macHeader->id;

    for (int i = 0; i < (Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH) * Config::BIT_LENGTH / Config::BAND_WIDTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(audio + i, tmp);
    out.addArray(bitToByte(tmp));

    m_isGood = AudioDevice::codec.decodeBlock(out, frameData, 0);

    /* Check CRC */
    macHeader_uint8 = frameData.getRawDataPointer();
    MACFrame *frame = reinterpret_cast<MACFrame *>(macHeader_uint8);
    m_isGood = m_isGood && MACManager::get().macFrameFactory->checkCRC(frame);
}

Frame::Frame(Frame &&other)
    : frameAudio(std::move(other.frameAudio)),
    frameData(std::move(other.frameData)),
    m_audioPos(std::exchange(other.m_audioPos, 0)),
    m_isACK(std::exchange(other.m_isACK, false)),
    m_isGood(std::exchange(other.m_isGood, false)), 
    m_dataLength(std::exchange(other.m_dataLength, 0)),
    m_id(std::exchange(other.m_id, 0))
{}

Frame::Frame(const Frame &other)
    : frameAudio(other.frameAudio),
    frameData(other.frameData),
    m_audioPos(other.m_audioPos),
    m_isACK(other.m_isACK),
    m_isGood(other.m_isGood), 
    m_dataLength(other.m_dataLength),
    m_id(other.m_id)
{}

void Frame::addToBuffer(RingBuffer<float> &buffer) const
{
    buffer.write(frameAudio.getReadPointer(0), frameAudio.getNumSamples());
}

void Frame::addSound(const AudioType &src)
{
    if (src.getNumSamples() + m_audioPos > frameAudio.getNumSamples())
    {
        std::cerr << "ERROR: frameAudio is full" << newLine;
        return;
    }
    frameAudio.copyFrom(0, m_audioPos, src, 0, 0, src.getNumSamples());
    m_audioPos += src.getNumSamples();
}

void Frame::addHeader()
{
    frameAudio.copyFrom(0, 0, Config::header, 0, 0, Config::HEADER_LENGTH);
    m_audioPos += Config::HEADER_LENGTH;
}

void Frame::getData(DataType &out) const
{
    out.addArray(frameData, 0, m_dataLength + sizeof(MACHeader));
}

void Frame::getData(uint8_t *out) const
{
    memcpy(out, frameData.getRawDataPointer(), m_dataLength + sizeof(MACHeader));
}

const bool Frame::isGoodFrame() const
{
    return m_isGood;
}

const bool Frame::isACK() const
{
    return m_isACK;
}

const uint8_t Frame::dataLength() const 
{
    return m_dataLength;
}

const uint8_t Frame::id() const
{
    return m_id;
}