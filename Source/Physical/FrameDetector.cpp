#include "Physical/Audio.h"
#include "Physical/FrameDetector.h"
#include "Physical/Modulator.h"
#include <fstream>
#include "Config.h"
#include "Utils/IOFunctions.hpp"
#include "MAC/ACK.h"

extern std::ofstream debug_file;

float recent_power(int N, const float *x)
{
    float sum = 0;
    for (int i = 0; i < N; i++)
        sum += x[i] * x[i];
    return sum;
}

FrameDetector::FrameDetector()
    : m_state(CK_HEADER),
    stopCountdown(Config::RECV_TIMEOUT * Config::SAMPLE_RATE),
    mkl_dot(std::bind(cblas_sdot, std::placeholders::_1, std::placeholders::_2, 1, std::placeholders::_3, 1)),
    power([](int N, const float* dummy, const float* data) { return recent_power(N, data); })
{}

void FrameDetector::checkHeader()
{
    const float *header = Config::header.getReadPointer(0);
    int offsetStart = headerOffset;
    
    for (; headerOffset + Config::HEADER_LENGTH < detectorBuffer.size() && stopCountdown >= 0; headerOffset++)
    {
        float dot = detectorBuffer.peek(mkl_dot, header, (std::size_t)Config::HEADER_LENGTH, headerOffset);
        float rec_power = detectorBuffer.peek(power, header, (std::size_t)Config::HEADER_LENGTH, headerOffset);
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
            detectorBuffer.discard((std::size_t)prevMaxPos + Config::HEADER_LENGTH);
            resetState();
            m_state = FD_HEADER;
        }
    }

    if (prevMaxPos == -1 && headerOffset >= Config::SAMPLE_RATE)
    {
        detectorBuffer.discard(Config::SAMPLE_RATE / 2);
        headerOffset -= (Config::SAMPLE_RATE / 2);
    }
}

void FrameDetector::resetState()
{
    /* Reset the state */
    headerOffset = 0;
    prevMaxPos = -1;
    prevMax = 0.0f;
    stopCountdown = Config::RECV_TIMEOUT * Config::SAMPLE_RATE;
}

void FrameDetector::detectAndGet(std::list<Frame> &received)
{
    float buffer[Config::BIT_PER_FRAME * Config::BIT_LENGTH];

    if (m_state == CK_HEADER)
        checkHeader();

    if (m_state == FD_HEADER && detectorBuffer.hasEnoughElem((std::size_t)Config::MACHEADER_LENGTH * Config::BIT_LENGTH))
    {
        detectorBuffer.read(buffer, Config::MACHEADER_LENGTH * Config::BIT_LENGTH);
        // received.emplace_back((const float *)buffer);
        DataType frameHeader = getMACHeader(buffer);
        MACHeader *macHeader = headerView(frameHeader.getRawDataPointer());
        if (macHeader->type == Config::ACK && MACLayerTransmitter::checkACK(macHeader))
        {
            received.push_back(ACK(macHeader));
            m_state = CK_HEADER;
        }
        else if (macHeader->type == Config::DATA && MACLayerReceiver::checkFrame(macHeader))
        {
            m_curHeader = macHeader;
            m_state = GET_DATA;
        }
    }

    if (m_state == GET_DATA)
    {
        constexpr int remaining = (Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH) * Config::BIT_LENGTH;
        detectorBuffer.read(buffer, remaining);
        received.emplace_back(m_curHeader, buffer);
        m_state = CK_HEADER;
    }
}

DataType FrameDetector::getMACHeader(const float *samples)
{
    DataType bitArray;
    for (int i = 0; i < Config::MACHEADER_LENGTH * Config::BIT_LENGTH / Config::BAND_WIDTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(samples + i, bitArray);
    return bitToByte(bitArray);
}

bool FrameDetector::isTimeout()
{
    return stopCountdown <= 0;
}

void FrameDetector::clear()
{
    resetState();
    powers.clear();
    dotproducts.clear();
    detectorBuffer.reset();
    m_state = CK_HEADER;
}

FrameDetector::~FrameDetector()
{}
