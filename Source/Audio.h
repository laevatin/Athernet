#pragma once

#include <JuceHeader.h>

#define PI acos(-1)

using namespace juce;

class AudioDevice : public AudioIODeviceCallback,
    private HighResolutionTimer
{
public:
    AudioDevice()
        : frameLength(48000), 
          headerLength(1000),
          bitLength(500),
          transFreq(5000)
    {}

    AudioDevice(int frame, int header, int bitLen, int freq)
        : frameLength(frame),
          headerLength(header),
          bitLength(bitLen),
          transFreq(freq)
    {}

    void beginTransmit()
    {
        startTimer(50);

        const ScopedLock sl(lock);
        createTransSound();
        recordedSound.clear();
        playingSampleNum = recordedSampleNum = 0;
        isRunning = true;
    }

    void setData(const Array<int> &data)
    {
        // Deep copy
        inputData = data;
    }

    int getOutput(int index) const
    {
        return outputData[index];
    }

    void hiResTimerCallback() override
    {
        if (isRunning && recordedSampleNum >= recordedSound.getNumSamples())
        {
            isRunning = false;
            stopTimer();

            auto* record = recordedSound.getReadPointer(0);
            auto* header = transSound.getReadPointer(0);
            auto* cos = cosSound.getReadPointer(0);
            float sum = 0.0f;
            float sum1 = 0.0f;
            float maxsum = 0.0f;
            int headerpos = 0;

            for (int i = 0; i < frameLength - headerLength; i++) {
                for (int j = 0; j < headerLength; j++) {
                    sum += record[i + j] * header[j];
                }
                if (sum > maxsum) {
                    maxsum = sum;
                    headerpos = i;
                }
                sum = 0.0f;
            }

            std::cout << "pos: " << headerpos << newLine;
            //std::cout << "ref: " << sum1 << newLine;
            int k = 0;

            for (int i = headerpos + headerLength; i < frameLength; i += bitLength) {
                for (int j = 0; j < bitLength; j++) {
                    sum += record[i + j] * cos[headerLength + j];
                    sum1 += header[i - headerpos + j] * header[headerLength + j];
                }
                std::cout << newLine;
                std::cout << "ref: " << sum1 << newLine;
                std::cout << "get: " << sum << newLine;

                std::cout << "output: ";
                if (sum > 0)
                    std::cout << 1 << " ";
                else 
                    std::cout << 0 << " ";
                std::cout << newLine;
                
                sum1 = 0.0f;

                if (sum > 0)
                    outputData.set(k++, 1);
                else
                    outputData.set(k++, 0);
                sum = 0.0f;
            }
            // std::cout << getMessageDescribingResult(latencySamples);
        }
    }

    String getMessageDescribingResult(int latencySamples)
    {
        String message;

        return message;
    }

    //==============================================================================
    void audioDeviceAboutToStart(AudioIODevice* device) override
    {
        isRunning = false;
        playingSampleNum = recordedSampleNum = 0;
        outputData.resize(inputData.size());

        recordedSound.setSize(1, (int)(sampleRate));
        recordedSound.clear();
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples) override
    {
        const ScopedLock sl(lock);

        if (isRunning)
        {
            auto* recordingBuffer = recordedSound.getWritePointer(0);
            auto* playBuffer = transSound.getReadPointer(0);

            for (int i = 0; i < numSamples; ++i)
            {
                if (recordedSampleNum < recordedSound.getNumSamples())
                {
                    auto inputSamp = 0.0f;

                    for (auto j = numInputChannels; --j >= 0;)
                        if (inputChannelData[j] != nullptr)
                            inputSamp += inputChannelData[j][i];

                    recordingBuffer[recordedSampleNum] = inputSamp;
                }

                ++recordedSampleNum;

                auto outputSamp = (playingSampleNum < transSound.getNumSamples()) ? playBuffer[playingSampleNum] : 0.0f;

                for (auto j = numOutputChannels; --j >= 0;)
                    if (outputChannelData[j] != nullptr)
                        outputChannelData[j][i] = outputSamp;

                ++playingSampleNum;
            }
        }
        else
        {
            // We need to clear the output buffers, in case they're full of junk..
            for (int i = 0; i < numOutputChannels; ++i)
                if (outputChannelData[i] != nullptr)
                    zeromem(outputChannelData[i], (size_t)numSamples * sizeof(float));
        }
    }

private:
    AudioBuffer<float> transSound, recordedSound;
    AudioBuffer<float> cosSound;
    Array<int> inputData, outputData;
    CriticalSection lock;

    int playingSampleNum = 0;
    int recordedSampleNum = 0;
    
    int headerLength;
    int frameLength;
    int bitLength;
    int transFreq;

    double sampleRate = 48000;
    bool isRunning = false;

    //==============================================================================
    // create a test sound which consists of a series of randomly-spaced audio spikes..
    void createTransSound()
    {
        auto length = ((int)sampleRate);
        transSound.setSize(1, length);
        cosSound.setSize(1, bitLength);
        transSound.clear();

        transFreq = 9234; // 1000 Hz carrier wave
        float amp = 0.7;
        int channelNum = 1;
        float dPhasePerSample = 2 * PI * ((float)transFreq / (float)sampleRate);
        float dPhasePerSampleHeader = 2 * PI * ((float)5923.0 / (float)sampleRate);
        float initPhase = 0;
        float data;

        for (int i = 0; i < headerLength; i++) {
            data = amp * cos(dPhasePerSampleHeader * i);
            // Write the sample into the output channel 
            transSound.setSample(0, i, data);
        }

        for (int i = 0; i < bitLength; i++) {
            data = amp * cos(dPhasePerSample * i);
            cosSound.setSample(0, i, data);
        }

        for (int i = 0; i * bitLength < length - headerLength; i += 1) {
            for (int j = 0; j < bitLength; j++) {
                //int a = inputData[i];
                data = amp * cos(dPhasePerSample * j + PI * inputData[i]);
                // Write the sample into the output channel 
                transSound.setSample(0, i * bitLength + j + headerLength, data);
            }

        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDevice)
};

//==============================================================================
class AudioIO
{
public:
    AudioIO()
    {
        audioDeviceManager.initialiseWithDefaultDevices(1, 1);
        audioDevice.reset(new AudioDevice());
    }

    ~AudioIO()
    {
        audioDeviceManager.removeAudioCallback(audioDevice.get());
        audioDevice.reset();
    }

    void startTransmit()
    {
        //std::cout << "test start" << newLine;
        audioDeviceManager.addAudioCallback(audioDevice.get());

        audioDevice->beginTransmit();
    }

    void setTransmitData(const Array<int>& data)
    {
        audioDevice->setData(data);
    }

    int getOutput(int i)
    {
        return audioDevice->getOutput(i);
    }

private:
    AudioDeviceManager audioDeviceManager;

    std::unique_ptr<AudioDevice> audioDevice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioIO)
};
