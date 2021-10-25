#include "Physical/Audio.h"
#include "Physical/Codec.h"
#include "Config.h"
#include <fstream>

static std::condition_variable finishcv;
static std::mutex cv_m;

static DataType tester;

Codec AudioDevice::codec;

AudioDevice::AudioDevice(enum AudioIO::state s) 
    : deviceState(s)
{}

AudioDevice::~AudioDevice()
{}

void AudioDevice::beginTransmit()
{
    const ScopedLock sl(lock);
    if (deviceState & AudioIO::SENDING)
    {
        sender.reset();

        isSending = true;
    }
    
    if (deviceState & AudioIO::RECEIVING)
    {
        receiver.reset();
        demodulator.clear();

        isReceiving = true;
    }
    startTimer(10);
}

void AudioDevice::hiResTimerCallback() 
{   
    if (isSending)
    {
        // check ack
    }

    if (isReceiving)
    {
        std::size_t len = receiver.size();
        float *buffer = new float[len];

        lock.enter();
        if (demodulator.demodulatorBuffer.hasEnoughSpace(len))
        {
            receiver.read(buffer, len);
            demodulator.demodulatorBuffer.write(buffer, len);
        }
        lock.exit();

        delete [] buffer;

        // check header
        // need a new timeout
        // if (demodulator.isTimeout())
        // {
            // isReceiving = false;
            // std::cout << "Receiving timed out" << newLine;
        // }
    }

    if (!isSending && !isReceiving)
    {
        stopTimer();
        std::cout << "Finishing...\n";
        finishcv.notify_one();
    }
}

void AudioDevice::sendFrame(const Frame &frame)
{
    ScopedLock sl(lock);
    frame.addToBuffer(sender);
}

void AudioDevice::audioDeviceAboutToStart(AudioIODevice* device)
{
    isSending = false;
    isReceiving = false;
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
            std::size_t size = sender.size();
            sender.read(outputChannelData[0], size);
            zeromem(outputChannelData[0] + size, ((size_t)numSamples - size) * sizeof(float));
            isSending = false;
        }
    }
    else
    {
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
            std::cerr << "No enough space to receive." << newLine;
            std::size_t avail = receiver.avail();
            receiver.write(inputChannelData[0], avail);
            isReceiving = false;
        }
    }
}

AudioIO::AudioIO()
{
    audioDeviceManager.initialiseWithDefaultDevices(1, 1);
    audioDevice.reset(new AudioDevice(Config::STATE));
}

AudioIO::~AudioIO()
{
    audioDeviceManager.removeAudioCallback(audioDevice.get());
    audioDevice.reset();
}

void AudioIO::startTransmit()
{
    std::cout << "selected mode: " << Config::STATE << "\n"
              << "start transmitting data...\n";

    if (Config::STATE & RECEIVING)
        macReceiver.reset(new MACLayerReceiver(audioDevice));    

    if (Config::STATE & SENDING)
        macTransmitter.reset(new MACLayerTransmitter(inputBuffer, audioDevice));

    audioDeviceManager.addAudioCallback(audioDevice.get());
    audioDevice->beginTransmit();

    std::unique_lock<std::mutex> cv_lk(cv_m);
    finishcv.wait(cv_lk);

    if (Config::STATE & RECEIVING)
    {
        macReceiver->stopMACThread();
        macReceiver.release();
    }

    if (Config::STATE & SENDING)
    {
        macTransmitter->stopMACThread();
        macTransmitter.release();
    }
}

void AudioIO::write(const DataType &data) 
{
    inputBuffer = data;
}

void AudioIO::read(DataType &data)
{
    // read from maclayer
}
