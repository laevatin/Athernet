#include "Physical/FrameDetector.h"
#include "Physical/Modulator.h"
#include <fstream>
#include "Config.h"
#include "Utils/IOFunctions.hpp"
#include "MAC/MACManager.h"
#include "Physical/Frame.h"

extern std::ofstream debug_file;

float dot_product(int length, const float *x, const float *y) {
    float sum = 0;
    for (int i = 0; i < length; i++)
        sum += x[i] * y[i];
    return sum;
}

FrameDetector::FrameDetector()
        : m_state(CK_HEADER) {}

void FrameDetector::checkHeader(RingBuffer<float> &detectorBuffer) {
    const float *header = Config::header.getReadPointer(0);

    for (; headerOffset + Config::HEADER_LENGTH < detectorBuffer.size(); headerOffset++) {
        float dot = detectorBuffer.peek(std::function <float(int, const float *, const float *)>(dot_product),
                                header, (std::size_t) Config::HEADER_LENGTH, headerOffset);

        if (dot > 2.5f && dot > prevMax) {
            prevMax = dot;
            prevMaxPos = headerOffset;
        }

        if (prevMaxPos != -1 && headerOffset - prevMaxPos > 200) {
#ifdef VERBOSE_PHY
            std::cout << "header found at: " << prevMaxPos << std::endl;
#endif
            /* Clean out the mess */
            detectorBuffer.discard((std::size_t) prevMaxPos + Config::HEADER_LENGTH);
            resetState();
            m_state = FD_HEADER;
            break;
        }
    }

    if (prevMaxPos == -1 && headerOffset >= Config::SAMPLE_RATE && m_state != FD_HEADER) {
        detectorBuffer.discard(Config::SAMPLE_RATE / 2);
        headerOffset -= (Config::SAMPLE_RATE / 2);
    }
}

void FrameDetector::resetState() {
    /* Reset the state */
    headerOffset = 0;
    prevMaxPos = -1;
    prevMax = 0.0f;
}

void FrameDetector::detectAndGet(RingBuffer<float> &detectorBuffer, std::list<Frame> &received) {
    float buffer[Config::BIT_PER_FRAME * Config::BIT_LENGTH / Config::BAND_WIDTH];
    static FrameHeader *frameHeader;

    if (m_state == CK_HEADER)
        checkHeader(detectorBuffer);

    if (m_state == FD_HEADER && detectorBuffer.hasEnoughElem(
            (std::size_t) Config::PHYHEADER_LENGTH * Config::BIT_LENGTH / Config::BAND_WIDTH)) {
        detectorBuffer.read(buffer, Config::PHYHEADER_LENGTH * Config::BIT_LENGTH / Config::BAND_WIDTH);
        tmp = getFrameHeader(buffer);
        frameHeader = reinterpret_cast<FrameHeader *>(tmp.getRawDataPointer());
#ifdef VERBOSE_PHY
        std::cout << "frame length: " << (int)frameHeader->length << "\n";
#endif
        if (frameHeader->length > Config::BIT_PER_FRAME || frameHeader->length == 0)
            m_state = CK_HEADER;
        else
            m_state = GET_DATA;
    }

    if (m_state == GET_DATA) {
        int remaining = frameHeader->length * 8 * Config::BIT_LENGTH / Config::BAND_WIDTH;
        if (detectorBuffer.size() >= remaining) {
            detectorBuffer.read(buffer, remaining);
            received.emplace_back(frameHeader, buffer);
            m_state = CK_HEADER;
        }
    }
}

DataType FrameDetector::getFrameHeader(const float *samples) {
    DataType bitArray;
    for (int i = 0; i < Config::PHYHEADER_LENGTH * Config::BIT_LENGTH / Config::BAND_WIDTH; i += Config::BIT_LENGTH)
        Modulator::demodulate(samples + i, bitArray);
    return bitToByte(bitArray);
}

void FrameDetector::clear() {
    resetState();
    m_state = CK_HEADER;
}
