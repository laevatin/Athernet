#include "Physical/Frame.h"
#include "Physical/Audio.h"
#include "Physical/Demodulator.h"
#include "Physical/Modulator.h"
#include <fstream>
#include "Config.h"

extern std::ofstream debug_file;

float recent_power(int N, const float *x)
{
    float sum = 0;
    for (int i = 0; i < N; i++)
        sum += x[i] * x[i];
    return sum;
}

Demodulator::Demodulator()
    : status(false),
    stopCountdown(Config::RECV_TIMEOUT * Config::SAMPLE_RATE),
    frameCountdown(0),
    mkl_dot(std::bind(cblas_sdot, std::placeholders::_1, std::placeholders::_2, 1, std::placeholders::_3, 1)),
    power([](int N, const float* dummy, const float* data) { return recent_power(N, data); })
{}

void Demodulator::checkHeader()
{
    const float *header = Config::header.getReadPointer(0);
    int offsetStart = headerOffset;
    
    for (; headerOffset + Config::HEADER_LENGTH < demodulatorBuffer.size() && stopCountdown >= 0; headerOffset++)
    {
        float dot = demodulatorBuffer.peek(mkl_dot, header, (std::size_t)Config::HEADER_LENGTH, headerOffset);
        float rec_power = demodulatorBuffer.peek(power, header, (std::size_t)Config::HEADER_LENGTH, headerOffset);
        debug_file << dot << "\t" << rec_power << "\n";
        dotproducts.push_back(dot);
        powers.push_back(rec_power);
        stopCountdown -= 1;
    }
    
    for (; offsetStart < headerOffset; offsetStart++)
    {
        int frac = dotproducts[offsetStart] / powers[offsetStart];
        if (dotproducts[offsetStart] > 3.0f && dotproducts[offsetStart] > prevMax)
        {
            prevMax = dotproducts[offsetStart];
            prevMaxPos = offsetStart;
        }
        if (prevMaxPos != -1 && offsetStart - prevMaxPos > 500)
        {
            std::cout << "header found at: " << prevMaxPos << std::endl;
            /* Clean out the mess */
            dotproducts.clear();
            powers.clear();
            demodulatorBuffer.discard((std::size_t)prevMaxPos + Config::HEADER_LENGTH);
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
    frameCountdown = Config::BIT_PER_FRAME;
    stopCountdown = Config::RECV_TIMEOUT * Config::SAMPLE_RATE;
}

void Demodulator::demodulate(std::list<Frame> &received)
{   
    float buffer[Config::SAMPLE_PER_FRAME];
    if (!status)
        checkHeader();
    
    if (status && demodulatorBuffer.hasEnoughElem((std::size_t)Config::SAMPLE_PER_FRAME))
    {
        demodulatorBuffer.read(buffer, Config::SAMPLE_PER_FRAME);
        received.emplace_back((const float *)buffer);
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
    powers.clear();
    dotproducts.clear();
    demodulatorBuffer.reset();
    status = false;
}

Demodulator::~Demodulator()
{}
