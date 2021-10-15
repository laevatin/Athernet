#include "Frame.h"
#include "Audio.h"
#include "Demodulator.h"
#include <fstream>

#define RECV_TIMEOUT 1

extern std::ofstream debug_file;

Demodulator::Demodulator()
    : status(false),
    stopCountdown(RECV_TIMEOUT * Frame::sampleRate),
    frameCountdown(0),
    mkl_dot(std::bind(cblas_sdot, std::placeholders::_1, std::placeholders::_2, 1, std::placeholders::_3, 1))
{
    bitBuffer = (float *)malloc(sizeof(float) * Frame::getBitLength());
}

void Demodulator::checkHeader()
{
    const float *header = Frame::getHeader();
    int headerLen = Frame::getHeaderLength();
    int offsetStart = headerOffset;
    
    for (; headerOffset + headerLen < demodulatorBuffer.size() && stopCountdown >= 0; headerOffset++)
    {
        float dot = demodulatorBuffer.peek(mkl_dot, header, (std::size_t)headerLen, headerOffset);
         debug_file << dot << "\n";
        dotproducts.push_back(dot);
        stopCountdown -= 1;
    }
    
    for (; offsetStart < headerOffset; offsetStart++)
    {
        if (dotproducts[offsetStart] > prevMax && dotproducts[offsetStart] > 3)
        {
            prevMax = dotproducts[offsetStart];
            prevMaxPos = offsetStart;
        }
        if (prevMaxPos != -1 && offsetStart - prevMaxPos > 500)
        {
            std::cout << "\nheader found at: " << prevMaxPos << std::endl;
            /* Clean out the mess */
            dotproducts.clear();
            demodulatorBuffer.discard((std::size_t)prevMaxPos + Frame::getHeaderLength());
            resetState();
            status = true;
        }
    }
}

void Demodulator::resetState()
{
    /* Reset the state */
    headerOffset = 0;
    prevMaxPos = -1;
    prevMax = 0.0f;
    frameCountdown = Frame::getBitPerFrame();
    stopCountdown = RECV_TIMEOUT * Frame::sampleRate;
}

void Demodulator::demodulate(DataType &dataOut)
{   
    int bitLen = Frame::getBitLength();
    if (!status)
        checkHeader();
    
    while (status && demodulatorBuffer.hasEnoughElem((std::size_t)bitLen))
    {
        demodulatorBuffer.read(bitBuffer, bitLen);
        Frame::demodulate(bitBuffer, dataOut);
        frameCountdown -= BAND_WIDTH;
        if (frameCountdown <= 0)
            status = false;
    }
}

bool Demodulator::isTimeout()
{
    return stopCountdown <= 0;
}

bool Demodulator::isGettingBit()
{
    return status;
}

void Demodulator::clear()
{
    resetState();
    dotproducts.clear();
    demodulatorBuffer.reset();
    status = false;
}

Demodulator::~Demodulator()
{
    free(bitBuffer);
}
