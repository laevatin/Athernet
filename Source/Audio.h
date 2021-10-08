#pragma once

#include <JuceHeader.h>

#define PI acos(-1)

using namespace juce;

class AudioDevice : public AudioIODeviceCallback,
    private HighResolutionTimer
{
public:
    AudioDevice()
    {}

    void beginTest()
    {
        startTimer(50);

        const ScopedLock sl(lock);
        createTestSound();
        recordedSound.clear();
        playingSampleNum = recordedSampleNum = 0;
        testIsRunning = true;
    }

    void hiResTimerCallback() override
    {
        std::cout << "timer callback" << newLine;
        if (testIsRunning && recordedSampleNum >= recordedSound.getNumSamples())
        {
            testIsRunning = false;
            stopTimer();

            // Test has finished, so calculate the result..
            auto latencySamples = calculateLatencySamples();

            std::cout << getMessageDescribingResult(latencySamples);
        }
    }

    String getMessageDescribingResult(int latencySamples)
    {
        String message;

        if (latencySamples >= 0)
        {
            message << newLine
                << "Results:" << newLine
                << latencySamples << " samples (" << String(latencySamples * 1000.0 / sampleRate, 1)
                << " milliseconds)" << newLine
                << "The audio device reports an input latency of "
                << deviceInputLatency << " samples, output latency of "
                << deviceOutputLatency << " samples." << newLine
                << "So the corrected latency = "
                << (latencySamples - deviceInputLatency - deviceOutputLatency)
                << " samples (" << String((latencySamples - deviceInputLatency - deviceOutputLatency) * 1000.0 / sampleRate, 2)
                << " milliseconds)";
        }
        else
        {
            message << newLine
                << "Couldn't detect the test signal!!" << newLine
                << "Make sure there's no background noise that might be confusing it..";
        }

        return message;
    }

    //==============================================================================
    void audioDeviceAboutToStart(AudioIODevice* device) override
    {
        testIsRunning = false;
        playingSampleNum = recordedSampleNum = 0;

        sampleRate = 48000;
        deviceInputLatency = device->getInputLatencyInSamples();
        deviceOutputLatency = device->getOutputLatencyInSamples();

        recordedSound.setSize(1, (int)(0.9 * sampleRate));
        recordedSound.clear();
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples) override
    {
        const ScopedLock sl(lock);

        if (testIsRunning)
        {
            auto* recordingBuffer = recordedSound.getWritePointer(0);
            auto* playBuffer = testSound.getReadPointer(0);

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

                auto outputSamp = (playingSampleNum < testSound.getNumSamples()) ? playBuffer[playingSampleNum] : 0.0f;

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
    AudioBuffer<float> testSound, recordedSound;
    Array<int> spikePositions;
    CriticalSection lock;

    int playingSampleNum = 0;
    int recordedSampleNum = -1;
    double sampleRate = 0.0;
    bool testIsRunning = false;
    int deviceInputLatency = 0, deviceOutputLatency = 0;

    //==============================================================================
    // create a test sound which consists of a series of randomly-spaced audio spikes..
    void createTestSound()
    {
        auto length = ((int)sampleRate) / 4;
        testSound.setSize(1, length);
        testSound.clear();

        int freq = 800; // Hz
        float amp = 0.7;
        int sampleRate = 48000;
        int channelNum = 1;
        float dPhasePerSample = 2 * PI * ((float)freq / (float)sampleRate);
        float initPhase = 0;
        float data;

        for (int i = 0; i < length; i++) {
            data = amp * sin(dPhasePerSample * i + initPhase);
            // Write the sample into the output channel 
            testSound.setSample(0, i, data);
        }

        //Random rand;

        //for (int i = 0; i < length; ++i)
        //    testSound.setSample(0, i, (rand.nextFloat() - rand.nextFloat() + rand.nextFloat() - rand.nextFloat()) * 0.06f);

        //spikePositions.clear();

        //int spikePos = 0;
        //int spikeDelta = 50;

        //while (spikePos < length - 1)
        //{
        //    spikePositions.add(spikePos);

        //    testSound.setSample(0, spikePos, 0.99f);
        //    testSound.setSample(0, spikePos + 1, -0.99f);

        //    spikePos += spikeDelta;
        //    spikeDelta += spikeDelta / 6 + rand.nextInt(5);
        //}
    }

    // Searches a buffer for a set of spikes that matches those in the test sound
    int findOffsetOfSpikes(const AudioBuffer<float>& buffer) const
    {
        return 0;
        auto minSpikeLevel = 5.0f;
        auto smooth = 0.975;
        auto* s = buffer.getReadPointer(0);
        int spikeDriftAllowed = 5;

        Array<int> spikesFound;
        spikesFound.ensureStorageAllocated(100);
        auto runningAverage = 0.0;
        int lastSpike = 0;

        for (int i = 0; i < buffer.getNumSamples() - 10; ++i)
        {
            auto samp = std::abs(s[i]);

            if (samp > runningAverage * minSpikeLevel && i > lastSpike + 20)
            {
                lastSpike = i;
                spikesFound.add(i);
            }

            runningAverage = runningAverage * smooth + (1.0 - smooth) * samp;
        }

        int bestMatch = -1;
        auto bestNumMatches = spikePositions.size() / 3; // the minimum number of matches required

        if (spikesFound.size() < bestNumMatches)
            return -1;

        for (int offsetToTest = 0; offsetToTest < buffer.getNumSamples() - 2048; ++offsetToTest)
        {
            int numMatchesHere = 0;
            int foundIndex = 0;

            for (int refIndex = 0; refIndex < spikePositions.size(); ++refIndex)
            {
                auto referenceSpike = spikePositions.getUnchecked(refIndex) + offsetToTest;
                int spike = 0;

                while ((spike = spikesFound.getUnchecked(foundIndex)) < referenceSpike - spikeDriftAllowed
                    && foundIndex < spikesFound.size() - 1)
                    ++foundIndex;

                if (spike >= referenceSpike - spikeDriftAllowed && spike <= referenceSpike + spikeDriftAllowed)
                    ++numMatchesHere;
            }

            if (numMatchesHere > bestNumMatches)
            {
                bestNumMatches = numMatchesHere;
                bestMatch = offsetToTest;

                if (numMatchesHere == spikePositions.size())
                    break;
            }
        }

        return bestMatch;
    }

    int calculateLatencySamples() const
    {
        // Detect the sound in both our test sound and the recording of it, and measure the difference
        // in their start times..
        auto referenceStart = findOffsetOfSpikes(testSound);
        jassert(referenceStart >= 0);

        auto recordedStart = findOffsetOfSpikes(recordedSound);

        return (recordedStart < 0) ? -1
            : (recordedStart - referenceStart);
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
    }

    ~AudioIO()
    {
        audioDeviceManager.removeAudioCallback(audioDevice.get());
        audioDevice.reset();
    }

    void startTest()
    {
        std::cout << "test start" << newLine;
        if (audioDevice.get() == nullptr)
        {
            audioDevice.reset(new AudioDevice());
            audioDeviceManager.addAudioCallback(audioDevice.get());
        }

        audioDevice->beginTest();
    }

private:
    AudioDeviceManager audioDeviceManager;

    std::unique_ptr<AudioDevice> audioDevice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioIO)
};
