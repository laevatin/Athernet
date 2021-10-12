#include "Demodulator.h"

Demodulator::Demodulator()
    :power(Frame::getHeaderLength())
{}

bool Demodulator::checkHeader()
{
    const float *header = Frame::getHeader();
    const float *received = headerBuffer.getReadPointer(0);
    int headerLen = Frame::getHeaderLength();
    int receivedLen = headerBuffer.getNumSamples();
    float *buffer = (float *)malloc(sizeof(float) * headerLen);

    if (receivedLen < headerLen)
        return false;

    for (int i = 0; i < receivedLen - headerLen; i++)
    {
        float f = cblas_sdot(headerLen, header, 1, received, 1);
        std::cout << f << " ";
    }
    
    memcpy(buffer, received + receivedLen - headerLen, headerLen);
    headerBuffer.clear();
    headerBuffer.copyFrom(0, 0, buffer, headerLen);
    return true;
}

Demodulator::~Demodulator()
{}
