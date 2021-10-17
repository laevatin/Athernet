#pragma once

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <JuceHeader.h>
#include <list>
#include "Frame.h"
#include "Ringbuffer.hpp"
#include "Demodulator.h"
#include <condition_variable>
#include <mutex>

#define PI acos(-1)

using namespace juce;

typedef Array<uint8_t> DataType;
typedef AudioBuffer<float> AudioType;

class Codec;

class AudioDevice : public AudioIODeviceCallback,
    private HighResolutionTimer
{   
public:

    enum state {
        SENDING = 1,
        RECEIVING = 2,
        BOTH = 3
    };

    AudioDevice();
    ~AudioDevice();

    void beginTransmit();

    void hiResTimerCallback() override;

    void setDeviceState(enum state s);

    void setSendData(DataType &input);

    DataType getRecvData();

    //==============================================================================
    void audioDeviceAboutToStart(AudioIODevice* device) override;

    void audioDeviceStopped() override;

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples) override;

protected:
    DataType inputData, outputData;
    int inputPos = 0;

private:
    CriticalSection lock;
    RingBuffer<float> sender, receiver;
    std::list<Frame> pendingFrames;

    Demodulator demodulator;
    Codec *codec;

    /* Send by default */
    enum state curState = SENDING;

    bool isSending = false;
    bool isReceiving = false;
    
    void createNextFrame();

    /* Append the sound from src to dest at start */
    void addTransSound(const AudioType &src, int channel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDevice)
};

//==============================================================================
class AudioIO
{
public:
    AudioIO();
    ~AudioIO();

    void startTransmit();
    void write(const DataType &data);
    void read(DataType &data);
    
private:
    AudioDeviceManager audioDeviceManager;

    std::unique_ptr<AudioDevice> audioDevice;

    DataType inputBuffer;
    DataType outputBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioIO)
};

#endif
