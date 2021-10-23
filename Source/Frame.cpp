#include "Frame.h"
#include "mkl.h"
#include <fstream>
#include "Config.h"
#include "Codec.h"
#include "Modulator.h"

std::ofstream debug_file;

static DataType bitToByte(const DataType &bitArray)
{
    DataType byteArray;
    byteArray.resize(bitArray.size() / 8);

    for (int i = 0; i < bitArray.size(); i += 8) 
    {
        uint8_t val = 0;
        for (int j = 0; j < 8; j++)
            val |= bitArray[i + j] << ((i + j) % 8);
        byteArray.set(i / 8, val);  
    }

    return std::move(byteArray);
}

static DataType byteToBit(const DataType &byteArray)
{
    DataType bitArray;

    bitArray.resize(byteArray.size() * 8);
    for (int i = 0; i < bitArray.size(); i++) 
    {
        bitArray.set(i, ((1 << (i % 8)) & (byteArray[i / 8])) >> (i % 8));
    }

    return std::move(bitArray);
}

Frame::Frame(const DataType &data, int start)
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);

    addHeader();
    
    DataType encoded = byteToBit(AudioDevice::codec.encodeBlock(bitToByte(data), start));
    Modulator::modulate(encoded, 0, *this);
}

Frame::Frame()
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);
    DataType data;
    data.resize(Config::BIT_PER_FRAME);
    Modulator::modulate(data, 0, *this);
}

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

Frame::~Frame()
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

void Frame::getData(DataType &out)
{
    out.addArray(frameData);
}