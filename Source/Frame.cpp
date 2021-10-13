#include "Frame.h"
#include "mkl.h"

int Frame::bitLen;
int Frame::headerLen;
int Frame::frameLen;
int Frame::freq;
int Frame::bitPerFrame;

std::vector<AudioType> Frame::modulateSound;

AudioType Frame::header;

Frame::Frame(const DataType &data, int start)
{
    frameAudio.setSize(1, frameLen);

    addHeader();
    modulate(data, start);
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

AudioType Frame::generateSound(int freq, int length, float initPhase)
{
    AudioType sound;
    sound.setSize(1, length);
    auto soundPointer = sound.getWritePointer(0);

    for (int i = 0; i < length; i++)
        soundPointer[i] = sin(i * 2 * PI * ((float)freq / (float)sampleRate) + initPhase);
    
    return std::move(sound);
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

int Frame::getBitLength()
{
    return Frame::bitLen;
}

const float *Frame::getHeader()
{
    return header.getReadPointer(0);
}

void Frame::modulate(const DataType &data, int start) 
{
    //for (int i = start; i < start + bitPerFrame; i++)
    //{
    //    /* gets 0 if i is out of bound */
    //    if (data[i] == 1) 
    //        addSound(Frame::modulateSound[0]);
    //    else 
    //        addSound(Frame::modulateSound[1]);
    //}
    for (int i = 0; i < frameLen; i++)
        frameAudio.setSample(0, i, 0);

}

/* consume bitLen samples, `samples` should contain at least bitLen data */
int8_t Frame::demodulate(const float *samples)
{
    float data = cblas_sdot(bitLen, samples, 1, modulateSound[0].getReadPointer(0), 1);
    const float thres1 = 0.5;
    const float thres0 = -0.5;

    std::cout << data << std::endl;

    if (data > thres1)
        return 1;
    else if (data < thres0)
        return 0;
}

void Frame::frameInit()
{
    for (int f = 800; f <= 6400; f *= 2)
    {
        modulateSound.emplace_back(generateSound(f, bitLen, 0));
        modulateSound.emplace_back(generateSound(f, bitLen, PI));
    }

    generateHeader();
}

void Frame::addSound(const AudioType &src)
{
    if (src.getNumSamples() + audioPos > frameAudio.getNumSamples())
    {
        std::cout << "ERROR: frameAudio is full" << newLine;
        return;
    }
    frameAudio.copyFrom(0, audioPos, src, 0, 0, src.getNumSamples());
    audioPos += src.getNumSamples();
}

void Frame::addHeader()
{
    frameAudio.copyFrom(0, 0, Frame::header, 0, 0, Frame::headerLen);
}