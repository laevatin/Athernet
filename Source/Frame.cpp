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

Frame::~Frame()
{}

void Frame::addToBuffer(RingBuffer<float> &buffer) const
{
    buffer.write(frameAudio.getReadPointer(0), Config::FRAME_LENGTH);
}

void Frame::modulate(const DataType &data, int start) 
{
    for (int i = start; i < start + Config::BIT_PER_FRAME; i += Config::BAND_WIDTH)
    {
        /* gets 0 if i is out of bound */
        int8_t composed = data[i];
        composed = composed | (data[i + 1] << 1);
        std::cout << (int)composed << " ";

        addSound(Config::modulateSound[composed]);
    }
	std::cout << newLine;
    // for (int i = headerLen; i < frameLen; i++)
    //     frameAudio.setSample(0, i, 0);
}

/* consume Config::BIT_LENGTH samples, `samples` should contain at least Config::BIT_LENGTH data */
void Frame::demodulate(const float *samples, DataType &out)
{
    float data[4];
    data[0] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[0].getReadPointer(0), 1);
    data[1] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[1].getReadPointer(0), 1);
    data[2] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[2].getReadPointer(0), 1);
    data[3] = cblas_sdot(Config::BIT_LENGTH, samples, 1, Config::modulateSound[3].getReadPointer(0), 1);

    float max = 0;
    int maxi = -1;

    for (int i = 0; i < 4; i++)
        if (data[i] > max)
        {
            max = data[i];
            maxi = i;
        }

    // std::cout << maxi << "\n";
    if (maxi == 0 || maxi == 2)
        out.add((int8_t)0);
    else
        out.add((int8_t)1);

    if (maxi == 2 || maxi == 3)
        out.add((int8_t)1);
    else
        out.add((int8_t)0);
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