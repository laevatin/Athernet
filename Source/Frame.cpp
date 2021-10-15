#include "Frame.h"
#include "mkl.h"
#include <fstream>

int Frame::bitLen;
int Frame::headerLen;
int Frame::frameLen;
int Frame::bitPerFrame;

std::ofstream debug_file;

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
    float startFreq = 6000;
    float endFreq = 16000;

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
        soundPointer[i] = sin((float)i * 2 * PI * ((float)freq / (float)sampleRate) + initPhase);
    
    return std::move(sound);
}

void Frame::setFrameProperties(int bitLen, int frameLen)
{
    Frame::bitLen = bitLen;
    Frame::frameLen = frameLen;
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
    for (int i = start; i < start + bitPerFrame; i += BAND_WIDTH)
    {
        /* gets 0 if i is out of bound */
        int8_t composed = data[i];
        composed = composed | (data[i + 1] << 1);
        std::cout << (int)composed << " ";

        addSound(modulateSound[composed]);
    }
	std::cout << newLine;
    // for (int i = headerLen; i < frameLen; i++)
    //     frameAudio.setSample(0, i, 0);
}

/* consume bitLen samples, `samples` should contain at least bitLen data */
void Frame::demodulate(const float *samples, DataType &out)
{
    float data[4];
    data[0] = cblas_sdot(bitLen, samples, 1, modulateSound[0].getReadPointer(0), 1);
    data[1] = cblas_sdot(bitLen, samples, 1, modulateSound[1].getReadPointer(0), 1);
    data[2] = cblas_sdot(bitLen, samples, 1, modulateSound[2].getReadPointer(0), 1);
    data[3] = cblas_sdot(bitLen, samples, 1, modulateSound[3].getReadPointer(0), 1);
    // const float thres1 = 0.5;
    // const float thres0 = -0.5;
    float max = 0;
    int maxi = -1;
    for (int i = 0; i < 4; i++)
        if (data[i] > max)
        {
            max = data[i];
            maxi = i;
        }
    std::cout << maxi << " ";
    if (maxi == 0 || maxi == 2)
        out.add((int8_t)0);
    else
        out.add((int8_t)1);

    if (maxi == 2 || maxi == 3)
        out.add((int8_t)1);
    else
        out.add((int8_t)0);
}

void Frame::frameInit()
{
    int base = 5600;
    AudioType audio1 = generateSound(base, bitLen, 0);
    AudioType audio2 = generateSound(base, bitLen, PI);
    AudioType audio3 = generateSound(base * 2, bitLen, 0);
    AudioType audio4 = generateSound(base * 2, bitLen, PI);

    AudioType tmp;
    tmp.setSize(1, bitLen);
    for (int i = 0; i < bitLen; i++)
        tmp.setSample(0, i, audio1.getSample(0, i) + audio3.getSample(0, i));
    modulateSound.push_back(tmp);
    for (int i = 0; i < bitLen; i++)
        tmp.setSample(0, i, audio1.getSample(0, i) + audio4.getSample(0, i));
    modulateSound.push_back(tmp);
    for (int i = 0; i < bitLen; i++)
        tmp.setSample(0, i, audio2.getSample(0, i) + audio3.getSample(0, i));
    modulateSound.push_back(tmp);
    for (int i = 0; i < bitLen; i++)
        tmp.setSample(0, i, audio2.getSample(0, i) + audio4.getSample(0, i));
    modulateSound.push_back(tmp);
    // for (int f = 2843; f <= 6400; f *= 2)
    // {
    //     modulateSound.emplace_back(generateSound(f, bitLen, 0));
    //     modulateSound.emplace_back(generateSound(f, bitLen, PI));
    // }

    generateHeader();
    Frame::bitPerFrame = (frameLen - headerLen) / bitLen * BAND_WIDTH;
}

void Frame::addSound(const AudioType &src)
{
    if (src.getNumSamples() + audioPos > frameAudio.getNumSamples())
    {
        std::cerr << "ERROR: frameAudio is full" << newLine;
        return;
    }
    frameAudio.copyFrom(0, audioPos, src, 0, 0, src.getNumSamples());
    audioPos += src.getNumSamples();
}

void Frame::addHeader()
{
    frameAudio.copyFrom(0, 0, Frame::header, 0, 0, Frame::headerLen);
    audioPos += headerLen;
}