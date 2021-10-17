#include "Frame.h"
#include "mkl.h"
#include <fstream>
#include "Config.h"

std::ofstream debug_file;

Frame::Frame(const DataType &data, int start)
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);

    addHeader();
    modulate(data, start);
}

Frame::Frame()
{
    frameAudio.setSize(1, Config::FRAME_LENGTH);
    DataType data;
    data.resize(Config::BIT_PER_FRAME);
    modulate(data, 0);
}

Frame::~Frame()
{}

void Frame::addToBuffer(RingBuffer<float> &buffer) const
{
    buffer.write(frameAudio.getReadPointer(0), frameAudio.getNumSamples());
}

void Frame::modulate(const DataType &data, int start) 
{
    for (int i = start; i < start + Config::BIT_PER_FRAME; i += Config::BAND_WIDTH)
    {
        /* gets 0 if i is out of bound */
        uint8_t composed = data[i];
        composed = composed | (data[i + 1] << 1);
        std::cout << (int)composed << " ";

        addSound(Config::modulateSound[composed]);
    }
	std::cout << newLine;
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