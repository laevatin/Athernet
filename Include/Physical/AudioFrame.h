#pragma once

#ifndef __AUDIOFRAME_H__
#define __AUDIOFRAME_H__

#include <JuceHeader.h>
#include "Utils/Ringbuffer.hpp"
#include "Frame.h"

typedef juce::Array<uint8_t> DataType;
typedef juce::AudioBuffer<float> AudioType;

class AudioFrame
{
public:
    /* Construct an audio frame with len bytes */
    AudioFrame(const void *data, size_t len);

    /* Trivial copy constructor */
    AudioFrame(const AudioFrame& other) = default;

    /* Move constructor */
    AudioFrame(AudioFrame&& other) noexcept;

    explicit AudioFrame(const Frame& frame);

    void addSound(const AudioType& src);
    void addToBuffer(RingBuffer<float>& buffer);

private:
    void addHeader();
    AudioType m_frameAudio;
    size_t m_audioPos;
};

#endif