#include "Physical/Frame.h"
#include "Physical/Codec.h"
#include "Physical/Modulator.h"
#include "MAC/MACFrame.h"
#include "Utils/IOFunctions.hpp"
#include "mkl.h"
#include <fstream>
#include "Config.h"

std::ofstream debug_file;

Frame::Frame(const uint8_t *pdata)
    : m_isGood(true)
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);
    DataType data;
    data.addArray(pdata, Config::DATA_PER_FRAME / 8);

    addHeader();
    DataType encoded = byteToBit(AudioDevice::codec.encodeBlock(data, 0));
    
    // for (int i = 0; i < encoded.size(); i += 1)
    //     std::cout << (int)encoded[i];
    // std::cout << "\n";

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

    for (int i = 0; i < (Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH) * Config::BIT_LENGTH / Config::BAND_WIDTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(audio + i, tmp);
    out.addArray(bitToByte(tmp));
    
    DataType ooo = byteToBit(out);

    // std::cout << "GETDATA: ";
    // for (int i = 0; i < ooo.size(); i += 1)
    //     std::cout << (int)ooo[i];
    // std::cout << "\n";

    m_isGood = AudioDevice::codec.decodeBlock(out, frameData, 0);
}

Frame::Frame(Frame &&other)
    : frameAudio(std::move(other.frameAudio)),
    frameData(std::move(other.frameData)),
    m_audioPos(std::exchange(other.m_audioPos, 0)),
    m_isACK(std::exchange(other.m_isACK, false)),
    m_isGood(std::exchange(other.m_isGood, false)), 
    m_dataLength(std::exchange(other.m_dataLength, 0))
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
