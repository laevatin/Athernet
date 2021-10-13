#include "Audio.h"
#include "Frame.h"

Demodulator::Demodulator()
    :power(Frame::getHeaderLength()),
    status(false)
{}

void Demodulator::checkHeader(RingBuffer<float> &receiver)
{
    const float *header = Frame::getHeader();
    int headerLen = Frame::getHeaderLength();
    float *buffer = (float *)malloc(sizeof(float) * headerLen);

    while (receiver.hasEnoughElem(headerLen))
    {
        receiver.peek(buffer, (std::size_t)headerLen);
        float f = cblas_sdot(headerLen, header, 1, buffer, 1);

        std::cout << f << newLine;

        // if (someSit) 
        // {
        //     frameCountdown = Frame::getBitPerFrame();
        //     status = true;
        //     break;
        // }

        receiver.pop();
    }

    free(buffer);
}

void Demodulator::demodulate(const float *samples, DataType &dataOut)
{
    if (!status)
        return;

    dataOut.add(Frame::demodulate(samples));
    frameCountdown -= 1;
    if (frameCountdown == 0)
        status = false;
}

bool Demodulator::isGettingBit()
{
    return status;
}

Demodulator::~Demodulator()
{}
