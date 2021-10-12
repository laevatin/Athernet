#include "Frame.h"

int Frame::bitLen;
int Frame::headerLen;
int Frame::frameLen;
int Frame::freq;
int Frame::bitPerFrame;

AudioType Frame::header;

Frame::Frame(const DataType &data, int start)
{
    frameAudio.setSize(1, frameLen);

    addHeader();
    modulate();
}

Frame::~Frame()
{}

const float *Frame::getReadPointer() const
{
    return frameAudio.getReadPointer(0);
}

void Frame::addToBuffer(RingBuffer<float> &buffer) const
{
    buffer.write(getReadPointer(), frameLen);
}

void Frame::generateHeader()
{
    // generate the header
    constexpr int headerLen = 440;
    float startFreq = 2000;
    float endFreq = 10000;

    float f_p[headerLen];
    float omega[headerLen];
    Frame::header.setSize(1, headerLen);
    Frame::headerLen = headerLen;

    f_p[0] = startFreq;
    for (int i = 1; i < headerLen / 2; i++)
        f_p[i] = f_p[i - 1] + (endFreq - startFreq) / (headerLen / 2);
    
    f_p[headerLen / 2] = endFreq;
    for (int i = headerLen / 2 + 1; i < headerLen; i++)
        f_p[i] = f_p[i - 1] - (endFreq - startFreq) / (headerLen / 2);

    omega[0] = 0;
    for (int i = 1; i < headerLen; i++)
        omega[i] = omega[i - 1] + (f_p[i] + f_p[i - 1]) / 2.0 * (1.0 / 48000);

    for (int i = 0; i < headerLen; i++)
        Frame::header.setSample(0, i, sin(2 * PI * omega[i]));
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

int Frame::getHeaderLength()
{
    return Frame::headerLen;
}

const float *Frame::getHeader()
{
    return header.getReadPointer(0);
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
    for (int i = 0; i < frameLen; i++) 
        frameAudio.addSample(0, i, 0);
}

void Frame::addHeader()
{
    frameAudio.copyFrom(0, 0, Frame::header, 0, 0, Frame::headerLen);
}