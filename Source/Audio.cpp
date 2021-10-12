#include "Audio.h"

AudioDevice::AudioDevice() 
{}

void AudioDevice::setDeviceState(enum state s)
{
    curState = s;
}

void AudioDevice::setSendData(DataType &input)
{
    inputData = std::move(input);
}

void AudioDevice::beginTransmit()
{
    startTimer(50);

    const ScopedLock sl(lock);

    if (curState & SENDING)
    {
        if (inputData.isEmpty())
        {
            std::cerr << "No Data!" << newLine;
            stopTimer();
            return;
        }

        sender.reset();
        while (inputPos < inputData.size() && pendingFrames.size() < PENDING_QUEUE_SIZE) 
            createNextFrame();
        
        isSending = true;
    }    
    
    if (curState & RECEIVING)
    {
        receiver.reset();
        isReceiving = true;
    }
}

void AudioDevice::hiResTimerCallback() 
{   
    if (isSending)
    {
        if (inputPos < inputData.size() && pendingFrames.size() < PENDING_QUEUE_SIZE) 
            createNextFrame();
        
        if (sender.hasEnoughSpace(Frame::getFrameLength()))
        {
            pendingFrames.front().addToBuffer(sender);
            pendingFrames.pop_front();
        }    
    }

    if (isReceiving)
    {
        // todo: Analyse header and demodulation
    }
}

void AudioDevice::audioDeviceAboutToStart(AudioIODevice* device)
{
    isSending = false;
    isReceiving = false;

    inputPos = 0;

    outputData.resize(inputData.size());
    outputData.clear();
}

void AudioDevice::audioDeviceStopped() {}

void AudioDevice::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples)
{
    const ScopedLock sl(lock);

    /* Only use channel 0. */
    if (isSending) 
    {
        if (sender.hasEnoughElem(numSamples)) 
            sender.read(outputChannelData[0], numSamples);
        else 
        {
            std::cout << "No enough audio to send." << newLine;
            std::size_t size = sender.size();
            sender.read(outputChannelData[0], size);
            zeromem(outputChannelData[0] + size, ((size_t)numSamples - size) * sizeof(float));
        }
    }
    else
    {
        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                zeromem(outputChannelData[i], (size_t)numSamples * sizeof(float));
    }

    if (isReceiving) 
    {
        if (receiver.hasEnoughSpace(numSamples))
            receiver.write(inputChannelData[0], numSamples);
        else
        {
            std::cout << "No enough space to receive" << newLine;
            int size = receiver.size();
            receiver.write(inputChannelData[0], size);
        }
    }
}

void AudioDevice::createNextFrame()
{
    pendingFrames.emplace_back(inputData, inputPos);
    inputPos += Frame::getBitPerFrame();
}

AudioIO::AudioIO()
{
    audioDeviceManager.initialiseWithDefaultDevices(1, 1);
    audioDevice.reset(new AudioDevice());
}

AudioIO::~AudioIO()
{
    audioDeviceManager.removeAudioCallback(audioDevice.get());
    audioDevice.reset();
}

void AudioIO::startTransmit()
{
    audioDevice->setDeviceState(AudioDevice::BOTH);
    audioDevice->setSendData(inputBuffer);
    audioDeviceManager.addAudioCallback(audioDevice.get());
    audioDevice->beginTransmit();
}

void AudioIO::write(const DataType &data) 
{
    inputBuffer = data;
}

int AudioIO::read(DataType &data)
{
    data = std::move(outputBuffer);
}
