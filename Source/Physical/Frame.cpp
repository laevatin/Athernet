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
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);
    DataType data;
    data.addArray(pdata, Config::BIT_PER_FRAME / 8);

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

    for (int i = 0; i < (Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH) * Config::BIT_LENGTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(audio + i, tmp);
    for (int i = 0; i < Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH; i += 1)
        std::cout << (int)tmp[i];
    std::cout << "\n";    

    out.addArray(bitToByte(tmp));
    frameData = AudioDevice::codec.decodeBlock(out, 0);
}

Frame::Frame(Frame &&other)
    : frameAudio(std::move(other.frameAudio)),
    frameData(std::move(other.frameData)),
    audioPos(std::exchange(other.audioPos, 0)),
    isACK(std::exchange(other.isACK, false))
{}

void Frame::addToBuffer(RingBuffer<float> &buffer) const
{
    buffer.write(frameAudio.getReadPointer(0), frameAudio.getNumSamples());
}

void Frame::addSound(const AudioType &src)
{
    if (src.getNumSamples() + audioPos > frameAudio.getNumSamples())
    {
        std::cerr << "ERROR: frameAudio is full" << newLine;
        return;
    }
    frameAudio.copyFrom(0, audioPos, src, 0, 0, src.getNumSamples());
    audioPos += src.getNumSamples();
}

void Frame::addHeader()
{
    frameAudio.copyFrom(0, 0, Config::header, 0, 0, Config::HEADER_LENGTH);
    audioPos += Config::HEADER_LENGTH;
}

void Frame::getData(DataType &out) const
{
    out.addArray(frameData);
}

void Frame::getData(uint8_t *out) const
{
    memcpy(out, frameData.getRawDataPointer(), frameData.size());
}