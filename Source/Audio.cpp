#include "Audio.h"
#include "Config.h"

static std::condition_variable cv;
static std::mutex cv_m;

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
    const ScopedLock sl(lock);
    if (curState & SENDING)
    {
        if (inputData.isEmpty())
        {
            std::cerr << "No Data!" << newLine;
            return;
        }

        inputPos = 0;
        sender.reset();
        pendingFrames.clear();

        while (inputPos < inputData.size() && pendingFrames.size() < Config::PENDING_QUEUE_SIZE) 
            createNextFrame();
        
        while (sender.hasEnoughSpace(Config::FRAME_LENGTH) && pendingFrames.size() > 0)
        {
            pendingFrames.front().addToBuffer(sender);
            pendingFrames.pop_front();
        }
        
        isSending = true;
    }
    
    if (curState & RECEIVING)
    {
        receiver.reset();
        outputData.clear();
        demodulator.clear();

        isReceiving = true;
    }
    startTimer(20);
}

void AudioDevice::hiResTimerCallback() 
{   
    if (isSending)
    {
        while (inputPos < inputData.size() && pendingFrames.size() < Config::PENDING_QUEUE_SIZE) 
            createNextFrame();
        
        lock.enter();
        while (sender.hasEnoughSpace(Config::FRAME_LENGTH) && pendingFrames.size() > 0)
        {
            pendingFrames.front().addToBuffer(sender);
            pendingFrames.pop_front();
        }
        lock.exit();
    }

    if (isReceiving)
    {
        std::size_t len1 = receiver.size();
        std::size_t len = std::min(len1, 12000);

        float *buffer = new float[len];

        lock.enter();
        if (demodulator.demodulatorBuffer.hasEnoughSpace(len))
        {
            receiver.read(buffer, len);
            demodulator.demodulatorBuffer.write(buffer, len);
        }
        lock.exit();

        delete [] buffer;

        demodulator.demodulate(outputData);
        if (demodulator.isTimeout())
        {
            isReceiving = false;
            std::cout << "Receiving timed out" << newLine;
        }
    }

    if (!isSending && !isReceiving)
    {
        stopTimer();
        std::cout << "Finishing...\n";
        cv.notify_all();
    }
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

void AudioDevice::createNextFrame()
{
    pendingFrames.emplace_back(inputData, inputPos);
    inputPos += Config::BIT_PER_FRAME;
}

DataType&& AudioDevice::getRecvData()
{
    return std::move(outputData);
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
    std::cout << "selected mode: " << Config::STATE << "\n"
              << "start transmitting data...\n";

    audioDevice->setDeviceState(Config::STATE);
    audioDevice->setSendData(inputBuffer);

    audioDeviceManager.addAudioCallback(audioDevice.get());
    audioDevice->beginTransmit();

    std::unique_lock<std::mutex> cv_lk(cv_m);
    cv.wait(cv_lk);
}

void AudioIO::write(const DataType &data) 
{
    inputBuffer = data;
}

void AudioIO::read(DataType &data)
{
    data = audioDevice->getRecvData();
}
