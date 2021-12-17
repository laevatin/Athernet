#include "Physical/AudioFrame.h"
#include "Utils/IOFunctions.hpp"
#include "Physical/Modulator.h"

AudioFrame::AudioFrame(const void *data, size_t len) {
    m_frameAudio.setSize(1, (Config::BIT_LENGTH * len * 8 / Config::BAND_WIDTH) + Config::HEADER_LENGTH + 10);
    DataType dataArray;
    dataArray.addArray(static_cast<const uint8_t *>(data), len);
    DataType bitArray = byteToBit(dataArray);
    addHeader();
    Modulator::modulate(*this, bitArray);
}

AudioFrame::AudioFrame(const Frame& frame)
    : AudioFrame(frame.m_frameData.getRawDataPointer(), frame.m_frameData.size()) {}

AudioFrame::AudioFrame(AudioFrame &&other) noexcept
    : m_frameAudio(std::move(other.m_frameAudio)),
      m_audioPos(std::exchange(other.m_audioPos, 0)) {}

void AudioFrame::addHeader() {
    m_frameAudio.copyFrom(0, 0, Config::header, 0, 0, Config::HEADER_LENGTH);
    m_audioPos += Config::HEADER_LENGTH;
}

void AudioFrame::addSound(const AudioType &src) {
#ifdef DEBUG
    if (src.getNumSamples() + m_audioPos > m_frameAudio.getNumSamples()) {
        std::cerr << "ERROR: m_frameAudio is full" << newLine;
        return;
    }
#endif
    m_frameAudio.copyFrom(0, m_audioPos, src, 0, 0, src.getNumSamples());
    m_audioPos += src.getNumSamples();
}

void AudioFrame::addToBuffer(RingBuffer<float> &buffer) const {
    buffer.write(m_frameAudio.getReadPointer(0), m_frameAudio.getNumSamples());
}
