#include "Audio.h"

AudioDevice::AudioDevice(int frame = 48000, int header = 1000, int bitLen = 500, int freq = 5000) 
  : frameLength(frame),
    headerLength(header),
    bitLength(bitLen),
    transFreq(freq)
{
    float dPhasePerSample = 2 * PI * ((float)transFreq / (float)sampleRate);
    float dPhasePerSampleHeader = 2 * PI * ((float)headerFreq / (float)sampleRate);

    cos0Sound.setSize(1, bitLength);
    cosPISound.setSize(1, bitLength);
    headerSound.setSize(1, headerLength);

    for (int i = 0; i < bitLength; i++) 
    {
        cos0Sound.setSample(0, i, cos(dPhasePerSample * i));
        cosPISound.setSample(0, i, cos(dPhasePerSample * i + PI));
    }

    for (int i = 0; i < headerLength; i++) 
        headerSound.setSample(0, i, cos(dPhasePerSampleHeader * i));

}

void AudioDevice::beginTransmit()
{
    if (inputData.isEmpty())
    {
        std::cout << "No Data!" << newLine;
        return;
    }

    startTimer(100);

    const ScopedLock sl(lock);
    createTransSound();
    recordedSound.clear();
    playingSampleNum = recordedSampleNum = 0;
    isRunning = true;
}

void AudioDevice::setTransmitData(const DataType &data)
{
    // Deep copy
    inputData = data;

    int len = inputData.size() * bitLength;
    framecnt = len / (frameLength - headerLength) + 1;

    duration = framecnt * frameLength;

    std::cout << "len: " << len << newLine;
    std::cout << "duration: " << duration << newLine;
}

int AudioDevice::getOutput(int index) const
{
    return outputData[index];
}

void AudioDevice::analyseRecord()
{
    auto* record = recordedSound.getReadPointer(0);
    auto* header = transSound.getReadPointer(0);
    auto* cos0 = cos0Sound.getReadPointer(0);

    float sum = 0.0f;
    float sum1 = 0.0f;
    float maxsum = 0.0f;
    int headerSearchStart = 800;

    int out = 0;

    for (int f = 0; f < framecnt; f++) 
    {
        int headerpos = findHeaderPos(headerSearchStart, 1000);
        int start = headerpos + headerLength; 
        int end = headerpos + frameLength;

        std::cout << "header pos: " << headerpos << newLine;
        for (int i = start; i < end; i += bitLength) {
            for (int j = 0; j < bitLength; j++) {
                sum += record[i + j] * cos0[j];
                sum1 += header[i - headerpos + j] * cos0[j];
            }
            std::cout << "ref: " << sum1 << " ";
            std::cout << "get: " << sum << " ";
            if (sum > 0)
                std::cout << 1 << " ";
            else 
                std::cout << 0 << " ";
            std::cout << newLine;
            sum1 = 0.0f;
            if (sum > 0)
                outputData.set(out++, 1);
            else
                outputData.set(out++, 0);
            sum = 0.0f;
        }
        headerSearchStart += frameLength;
    }
}

int AudioDevice::findHeaderPos(int start, int len)
{
    auto* record = recordedSound.getReadPointer(0);
    auto* header = headerSound.getReadPointer(0);

    float sum = 0.0f;
    float maxsum = 0.0f;
    int headerpos = 0;

    for (int i = start; i < start + len; i++) {
        for (int j = 0; j < headerLength; j++) {
            sum += record[i + j] * header[j];
        }
        if (sum > maxsum) {
            maxsum = sum;
            headerpos = i;
        }
        sum = 0.0f;
    }

    return headerpos;
}

void AudioDevice::hiResTimerCallback() 
{
    if (isRunning && recordedSampleNum >= recordedSound.getNumSamples())
    {
        // Finish transmiting
        isRunning = false;
        stopTimer();
        analyseRecord();
    }
}

void AudioDevice::audioDeviceAboutToStart(AudioIODevice* device)
{
    isRunning = false;
    playingSampleNum = recordedSampleNum = 0;
    transPos = 0;

    outputData.resize(inputData.size());
    outputData.clear();

    recordedSound.setSize(1, duration + headerLength + 5000);
    recordedSound.clear();
}

void AudioDevice::audioDeviceStopped() {}

void AudioDevice::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples)
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

void AudioDevice::createTransSound()
{
    transSound.setSize(1, duration);
    transSound.clear();

    float dPhasePerSample = 2 * PI * ((float)transFreq / (float)sampleRate);
    float dPhasePerSampleHeader = 2 * PI * ((float)headerFreq / (float)sampleRate);
    float data;
    int bitPerFrame = (frameLength - headerLength) / bitLength;
    int curBit = 0;

    for (int i = 0; i < framecnt; i++) 
    {
        addTransSound(headerSound, 0);
        for (int j = 0; j < bitPerFrame; j++)
        {
            if (inputData[curBit++]) 
                addTransSound(cos0Sound, 0);
            else 
                addTransSound(cosPISound, 0);
        }
    } 
    
    // for (int i = 0; i < headerLength; i++) {
    //     data = cos(dPhasePerSampleHeader * i);
    //     transSound.setSample(0, i, data);
    // }
    
    // for (int i = 0; i * bitLength < frameLength; i += 1) {
    //     for (int j = 0; j < bitLength; j++) {
    //         data = cos(dPhasePerSample * j + PI * inputData[i]);
    //         // Write the sample into the output channel 
    //         transSound.setSample(0, i * bitLength + j + headerLength, data);
    //     }
    // }
}

void AudioDevice::addTransSound(const AudioType &src, int channel)
{
    if (src.getNumSamples() + transPos > transSound.getNumSamples())
    {
        std::cout << "ERROR: transSound is full" << newLine;
        return;
    }
    transSound.copyFrom(channel, transPos, src, channel, 0, src.getNumSamples());
    transPos += src.getNumSamples();
}

AudioIO::AudioIO()
{
    audioDeviceManager.initialiseWithDefaultDevices(1, 1);
    if (!audioDevice.get())
        audioDevice.reset(new AudioDevice(48000, 1000, 500, 9127));
}

AudioIO::~AudioIO()
{
    audioDeviceManager.removeAudioCallback(audioDevice.get());
    audioDevice.reset();
}

void AudioIO::startTransmit()
{
    audioDeviceManager.addAudioCallback(audioDevice.get());
    audioDevice->beginTransmit();
}

void AudioIO::setTransmitData(const DataType &data)
{
    audioDevice->setTransmitData(data);
}

int AudioIO::getOutput(int i)
{
    return audioDevice->getOutput(i);
}