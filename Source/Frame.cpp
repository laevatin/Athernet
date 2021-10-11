#include "Frame.h"

Frame::Frame(const Array<int8_t> &data, int start, int len)
{
    frameAudio.setSize(0, frameLen);

    addHeader();
    modulate();
}

const float *Frame::getReadPointer() const
{
    return frameAudio.getReadPointer(0);
}

void Frame::addToBuffer(RingBuffer<float> &buffer) const
{
    buffer.write(getReadPointer(), frameLen);
}

void Frame::setHeader(const AudioBuffer<float> &header)
{
    Frame::header = header;
}

void Frame::setFrameProperties(int bitLen, int headerLen, int frameLen, int freq)
{
    Frame::bitLen = bitLen;
    Frame::headerLen = headerLen;
    Frame::frameLen = frameLen;
    Frame::freq = freq;
}

