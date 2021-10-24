#include "Physical/Frame.h"
#include "Physical/Codec.h"
#include "Physical/Modulator.h"
#include "Utils/IOFunctions.hpp"
#include "mkl.h"
#include <fstream>
#include "Config.h"

std::ofstream debug_file;

Frame::Frame(const DataType &data, int start)
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);

    addHeader();
    
    DataType encoded = byteToBit(AudioDevice::codec.encodeBlock(bitToByte(data), start));
    Modulator::modulate(encoded, 0, Config::BIT_PER_FRAME, *this);
}

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

Frame::Frame(const float *audio)
{
    DataType out;
    for (int i = 0; i < Config::SAMPLE_PER_FRAME; i += Config::BIT_LENGTH)
    {
        Modulator::demodulate(audio + i, out);
    }
    for (int i = 0; i < Config::BIT_PER_FRAME; i += 1)
    {
        std::cout << (int)out[i];
    }
    std::cout << "\n";
    frameData = byteToBit(AudioDevice::codec.decodeBlock(bitToByte(out), 0));
}

Frame::Frame(Frame &&other)
    : frameAudio(std::move(other.frameAudio)),
    frameData(std::move(other.frameData)),
    audioPos(std::exchange(other.audioPos, 0))
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