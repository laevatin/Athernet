#include "Physical/Audio.h"
#include "Physical/Codec.h"
#include "MAC/MACManager.h"
#include "Config.h"

#include <fstream>
#include <atomic>

static std::condition_variable finishcv;
static std::mutex cv_m;

Codec AudioDevice::codec;

extern std::ofstream debug_file;

// #define TEST_NOPHYS

AudioDevice::AudioDevice(enum state s) 
    : deviceState(s)
{}

AudioDevice::~AudioDevice()
{}

void AudioDevice::beginTransmit()
{
    const ScopedLock sl(lock);
    if (deviceState & SENDING)
    {
        sender.reset();

        isSending = true;
    }
    
    if (deviceState & RECEIVING)
    {
        receiver.reset();
        frameDetector.clear();

        isReceiving = true;
    }
    startTimer(10);
}

void AudioDevice::hiResTimerCallback() 
{   
    if (isSending || isReceiving)
    {
        std::size_t len = receiver.size();
        std::size_t maxlen = 12000;
        len = std::min(len, maxlen);
        float *buffer = new float[len];

        lock.enter();
        if (frameDetector.detectorBuffer.hasEnoughSpace(len))
        {
            receiver.read(buffer, len);
            frameDetector.detectorBuffer.write(buffer, len);
        }
        lock.exit();

        frameDetector.detectAndGet(received);

        if (!received.empty())
        {
            if (received.front().isACK() && (deviceState & SENDING)) 
                MACManager::get().macTransmitter->ACKReceived(std::move(received.front()));
            else if (!received.front().isACK() && (deviceState & RECEIVING))
                MACManager::get().macReceiver->frameReceived(std::move(received.front()));
            received.pop_front();
        }

        delete [] buffer;
    }

        // check header
        // need a new timeout
        // if (demodulator.isTimeout())
        // {
            // isReceiving = false;
            // std::cout << "Receiving timed out" << newLine;
        // }

    if (!isSending && !isReceiving)
    {
        stopTimer();
        finishcv.notify_one();
    }

#ifdef TEST_NOPHYS
    // simulate physical layer
    if (isSending && isReceiving)
    {
        lock.enter();
        // std::cout << "SEND" << "\n";
        int numSamples = 5000;
        float* buffer = new float[numSamples];
        if (sender.hasEnoughElem(numSamples))
            sender.read(buffer, numSamples);
        else
        {
            std::size_t size = sender.size();
            sender.read(buffer, size);
            zeromem(buffer + size, ((size_t)numSamples - size) * sizeof(float));
        }
 
        if (receiver.hasEnoughSpace(numSamples))
            receiver.write(buffer, numSamples);
        else
        {
            std::cerr << "No enough space to receive." << newLine;
            std::size_t avail = receiver.avail();
            receiver.write(buffer, avail);
            isReceiving = false;
        }
        lock.exit();
        delete[] buffer;
    }
#endif
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
#ifndef TEST_NOPHYS
    /* Only use channel 0. */
    if (isSending || isReceiving) 
    {
        if (sender.hasEnoughElem(numSamples)) 
            sender.read(outputChannelData[0], numSamples);
        else 
        {
            std::size_t size = sender.size();
            sender.read(outputChannelData[0], size);
            zeromem(outputChannelData[0] + size, ((size_t)numSamples - size) * sizeof(float));
        }

        if (receiver.hasEnoughSpace(numSamples))
            receiver.write(inputChannelData[0], numSamples);
        else
        {
            std::cerr << "No enough space to receive." << newLine;
            std::size_t avail = receiver.avail();
            receiver.write(inputChannelData[0], avail);
            isReceiving = false;
            isSending = false;
        }

        // Calculate the recent average power
        float sumPower = 0;
        for (int i = numSamples - Config::POWER_AVG_LEN; i < numSamples; i++) 
            sumPower += inputChannelData[0][i] * inputChannelData[0][i];
        m_avgPower = sumPower / Config::POWER_AVG_LEN;
        debug_file << m_avgPower << "\n";
    }
    else
    {
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                zeromem(outputChannelData[i], (size_t)numSamples * sizeof(float));
    }
#endif
}

void AudioDevice::stopReceiving()
{
    isReceiving = false;
}

void AudioDevice::stopSending()
{
    isSending = false;
}

enum AudioDevice::ChannelState AudioDevice::getChannelState() 
{
    if (m_avgPower > Config::POWER_THOR)
        return CN_BUSY;
    return CN_IDLE;
}

AudioIO::AudioIO()
{
    audioDeviceManager.initialiseWithDefaultDevices(1, 1);
    AudioDeviceManager::AudioDeviceSetup dev_info = audioDeviceManager.getAudioDeviceSetup();
    dev_info.sampleRate = 48000;
    dev_info.bufferSize = 512;
    audioDevice.reset(new AudioDevice(Config::STATE));
}

AudioIO::~AudioIO()
{
    audioDeviceManager.removeAudioCallback(audioDevice.get());
    audioDevice.reset();
}

void AudioIO::startTransmit()
{
    std::unique_ptr<MACLayerReceiver> macReceiver;
    std::unique_ptr<MACLayerTransmitter> macTransmitter;
    std::unique_ptr<CSMASenderQueue> csmaSenderQueue;

    std::cout << "selected mode: " << Config::STATE << "\n"
              << "start transmitting data...\n";
    
    if (Config::STATE & RECEIVING)
        macReceiver.reset(new MACLayerReceiver(audioDevice));    

    if (Config::STATE & SENDING)
        macTransmitter.reset(new MACLayerTransmitter(inputBuffer, audioDevice));

    csmaSenderQueue.reset(new CSMASenderQueue(audioDevice));

    MACManager::initialize(std::move(macReceiver), std::move(macTransmitter), std::move(csmaSenderQueue));

    audioDeviceManager.addAudioCallback(audioDevice.get());
    audioDevice->beginTransmit();

    std::unique_lock<std::mutex> cv_lk(cv_m);
    finishcv.wait(cv_lk);
    
    if (Config::STATE & RECEIVING)
        MACManager::get().macReceiver->getOutput(outputBuffer);
    MACManager::destroy();

    std::cout << "---------------- Transfer Finished ----------------\n"; 
}

void AudioIO::startPing()
{
    std::unique_ptr<MACLayerReceiver> macReceiver;
    std::unique_ptr<MACLayerTransmitter> macTransmitter;
    std::unique_ptr<CSMASenderQueue> csmaSenderQueue;

    std::cout << "selected mode: " << Config::STATE << "\n"
              << "start pinging...\n";
    
    if (Config::STATE & RECEIVING)
        macReceiver.reset(new MACLayerReceiver(audioDevice));    

    if (Config::STATE & SENDING)
        macTransmitter.reset(new MACLayerTransmitter(audioDevice));

    csmaSenderQueue.reset(new CSMASenderQueue(audioDevice));

    MACManager::initialize(std::move(macReceiver), std::move(macTransmitter), std::move(csmaSenderQueue));

    audioDeviceManager.addAudioCallback(audioDevice.get());
    audioDevice->beginTransmit();

    std::unique_lock<std::mutex> cv_lk(cv_m);
    finishcv.wait(cv_lk);
    
    if constexpr (Config::STATE & RECEIVING)
        MACManager::get().macReceiver->getOutput(outputBuffer);
    MACManager::destroy();

    std::cout << "---------------- Transfer Finished ----------------\n"; 

}

void AudioIO::write(const DataType &data) 
{
    inputBuffer = data;
}

void AudioIO::read(DataType &data)
{
    data = std::move(outputBuffer);
}
