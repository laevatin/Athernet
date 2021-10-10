#pragma once

#include <JuceHeader.h>

#define PI acos(-1)

using namespace juce;

class AudioDevice : public AudioIODeviceCallback,
    private HighResolutionTimer
{   
    typedef AudioBuffer<float> AudioType;
    typedef Array<int8_t> DataType;

public:
    AudioDevice(int frame, int header, int bitLen, int freq);

    void beginTransmit();

    void setTransmitData(const DataType &data);

    int getOutput(int index) const;

    void hiResTimerCallback() override;

    //==============================================================================
    void audioDeviceAboutToStart(AudioIODevice* device) override;

    void audioDeviceStopped() override;

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples) override;

private:
    
    AudioType transSound, recordedSound;
    AudioType cos0Sound, cosPISound, headerSound;
    DataType inputData, outputData;
    CriticalSection lock;

    int playingSampleNum = 0;
    int recordedSampleNum = 0;
    
    const int headerLength;
    const int frameLength;
    const int bitLength;
    const int transFreq;
    const int headerFreq = 5923;

    int framecnt = 0;
    int duration = 0;
    int transPos = 0;

    const double sampleRate = 48000;
    bool isRunning = false;
    
    void createTransSound();

    /* Append the sound from src to dest at start */
    void addTransSound(const AudioType &src, int channel);
    
    void analyseRecord();

    int findHeaderPos(int start, int len);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDevice)
};

//==============================================================================
class AudioIO
{
    typedef Array<int8_t> DataType;

public:
    AudioIO();
    ~AudioIO();

    void startTransmit();
    void setTransmitData(const DataType &data);
    int getOutput(int i);
    
private:
    AudioDeviceManager audioDeviceManager;

    std::unique_ptr<AudioDevice> audioDevice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioIO)
};
