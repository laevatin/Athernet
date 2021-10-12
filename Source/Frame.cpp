#include "Frame.h"

Frame::Frame(const Array<int8_t> &data, int start)
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

void Frame::setHeader(const AudioBuffer<float> &header, int headerLen)
{
    Frame::header = header;
    Frame::headerLen = headerLen;
}

void Frame::setFrameProperties(int bitLen, int frameLen, int freq)
{
    Frame::bitLen = bitLen;
    Frame::frameLen = frameLen;
    Frame::freq = freq;
    Frame::bitPerFrame = (frameLen - headerLen) / bitLen;
}

int Frame::getBitPerFrame()
{
    return Frame::bitPerFrame;
}

int Frame::getFrameLength()
{
    return Frame::frameLen;
}

void Frame::modulate() 
{
    // float dPhasePerSample = 2 * PI * ((float)freq / (float)sampleRate);
    // float dPhasePerSampleHeader = 2 * PI * ((float)headerFreq / (float)sampleRate);

    // cos0Sound.setSize(1, bitLength);
    // cosPISound.setSize(1, bitLength);
    // headerSound.setSize(1, headerLength);

    // for (int i = 0; i < bitLength; i++) 
    // {
    //     cos0Sound.setSample(0, i, cos(dPhasePerSample * i));
    //     cosPISound.setSample(0, i, cos(dPhasePerSample * i + PI));
    // }

    // for (int i = 0; i < headerLength; i++) 
    //     headerSound.setSample(0, i, cos(dPhasePerSampleHeader * i));
}

void Frame::addHeader()
{

}