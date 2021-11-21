#include "Physical/Audio.h"
#include "Physical/FrameDetector.h"
#include "Physical/Modulator.h"
#include <fstream>
#include "Config.h"
#include "Utils/IOFunctions.hpp"
#include "MAC/ACK.h"
#include "MAC/MACManager.h"

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
    mkl_dot(std::bind(cblas_sdot, std::placeholders::_1, std::placeholders::_2, 1, std::placeholders::_3, 1)),
    power([](int N, const float* dummy, const float* data) { return recent_power(N, data); })
{}

void FrameDetector::checkHeader()
{
    const float *header = Config::header.getReadPointer(0);
    int offsetStart = headerOffset;
    
    for (; headerOffset + Config::HEADER_LENGTH < detectorBuffer.size(); headerOffset++)
    {
        float dot = detectorBuffer.peek(mkl_dot, header, (std::size_t)Config::HEADER_LENGTH, headerOffset);
        //debug_file << dot << "\t" << rec_power << "\n";

        if (dot > 5.0f && dot > prevMax)
        {
            prevMax = dot;
            prevMaxPos = headerOffset;
        }

        if (prevMaxPos != -1 && headerOffset - prevMaxPos > 500)
        {
            //std::cout << "header found at: " << prevMaxPos << std::endl;
            /* Clean out the mess */
            detectorBuffer.discard((std::size_t)prevMaxPos + Config::HEADER_LENGTH);
            resetState();
            m_state = FD_HEADER;
            break;
        }
    }
    
    if (prevMaxPos == -1 && headerOffset >= Config::SAMPLE_RATE && m_state != FD_HEADER)
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
}

void FrameDetector::detectAndGet(std::list<Frame> &received)
{
    float buffer[Config::BIT_PER_FRAME * Config::BIT_LENGTH / Config::BAND_WIDTH];
    static MACHeader *macHeader;
    static uint8_t lastReceived = -1;

    if (m_state == CK_HEADER)
        checkHeader();

    if (m_state == FD_HEADER && detectorBuffer.hasEnoughElem((std::size_t)Config::MACHEADER_LENGTH * Config::BIT_LENGTH / Config::BAND_WIDTH))
    {
        detectorBuffer.read(buffer, Config::MACHEADER_LENGTH * Config::BIT_LENGTH / Config::BAND_WIDTH);
        // received.emplace_back((const float *)buffer);
        frameHeader = getMACHeader(buffer);
        macHeader = headerView(frameHeader.getRawDataPointer());
        if (macHeader->type == Config::ACK && MACLayerTransmitter::checkACK(macHeader))
        {
            std::cout << "RECIVER: FrameDetector: ACK detected, id: " << (int)macHeader->id << "\n";
            received.push_back(ACK(macHeader));
            m_state = CK_HEADER;
        }
        else if (macHeader->type == Config::DATA && MACLayerReceiver::checkFrame(macHeader))
        {
            std::cout << "RECIVER: FrameDetector: DATA detected, id: " << (int)macHeader->id
                << " length: " << (int)macHeader->len << "\n";
            if (macHeader->id == lastReceived)
            {
                std::cout << "RECIVER: This frame has been received\n";
                MACManager::get().macReceiver->sendACK(macHeader->id);
                m_state = CK_HEADER;
            }
            else 
                m_state = GET_DATA;
        }
		else if (macHeader->type == Config::MACPING_REQ && MACLayerTransmitter::checkPingReq(macHeader))
		{
			std::cout << "RECIVER: FrameDetector: MACPING_REQ detected, id: " << (int)macHeader->id << "\n";
			MACManager::get().macReceiver->sendACK(macHeader->id);
			m_state = CK_HEADER;
		}
        else
            m_state = CK_HEADER;
    }

    if (m_state == GET_DATA)
    {
        constexpr int remaining = (Config::BIT_PER_FRAME - Config::MACHEADER_LENGTH) * Config::BIT_LENGTH / Config::BAND_WIDTH;
        // std::cout << "detectorBuffer.size()" << detectorBuffer.size() << "\n";
        if (detectorBuffer.size() >= remaining) {
            detectorBuffer.read(buffer, remaining);
            received.emplace_back(macHeader, buffer);
            m_state = CK_HEADER;
            lastReceived = received.front().isGoodFrame() ? macHeader->id : -1;
        }
    }
}

DataType FrameDetector::getMACHeader(const float *samples)
{
    DataType bitArray;
    for (int i = 0; i < Config::MACHEADER_LENGTH * Config::BIT_LENGTH / Config::BAND_WIDTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(samples + i, bitArray);
    return bitToByte(bitArray);
}

void FrameDetector::clear()
{
    resetState();
    detectorBuffer.reset();
    m_state = CK_HEADER;
}

FrameDetector::~FrameDetector()
{}
