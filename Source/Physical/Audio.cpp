#include "Physical/Audio.h"
#include "Physical/Codec.h"
#include "MAC/MACManager.h"
#include "Config.h"

#include <fstream>
#include <atomic>

Codec AudioDevice::codec;

extern std::ofstream debug_file;

// #define TEST_NOPHYS

AudioDevice::AudioDevice(enum state s)
        : deviceState(s) {}

void AudioDevice::beginTransmit() {
    const ScopedLock sl(lock);
    if (deviceState & SENDING) {
        sender.reset();
        isSending = true;
    }

    if (deviceState & RECEIVING) {
        receiver.reset();
        frameDetector.clear();
        isReceiving = true;
    }
    startTimer(10);
}

void AudioDevice::hiResTimerCallback() {
#ifdef TEST_NOPHYS
    // simulate physical layer
    if (isSending && isReceiving)
    {
        lock.enter();
        // std::cout << "SEND" << "\n";
        int numSamples = 500;
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

void AudioDevice::sendFrame(const AudioFrame &frame) {
    ScopedLock sl(lock);
    frame.addToBuffer(sender);
}

void AudioDevice::audioDeviceAboutToStart(AudioIODevice *device) {
    isSending = false;
    isReceiving = false;
}

void AudioDevice::audioDeviceStopped() {}

void AudioDevice::audioDeviceIOCallback(const float **inputChannelData, int numInputChannels,
                                        float **outputChannelData, int numOutputChannels, int numSamples) {
    const ScopedLock sl(lock);
#ifndef TEST_NOPHYS
    /* Only use channel 0. */
    if (isSending || isReceiving) {
        if (sender.hasEnoughElem(numSamples))
            sender.read(outputChannelData[0], numSamples);
        else {
            std::size_t size = sender.size();
            sender.read(outputChannelData[0], size);
            zeromem(outputChannelData[0] + size, ((size_t) numSamples - size) * sizeof(float));
        }

        if (receiver.hasEnoughSpace(numSamples))
            receiver.write(inputChannelData[0], numSamples);
        else {
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

        frameDetector.detectAndGet(receiver, received);

        if (!received.empty()) {
            MACManager::get().macReceiver->frameReceived(std::move(received.front()));
            received.pop_front();
        }
    } else {
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                zeromem(outputChannelData[i], (size_t) numSamples * sizeof(float));
    }
#endif
}

enum AudioDevice::ChannelState AudioDevice::getChannelState() {
    if (m_avgPower > Config::POWER_THOR)
        return CN_BUSY;
    return CN_IDLE;
}

std::atomic<int> AudioIO::refcount = 0;
AudioDeviceManager AudioIO::audioDeviceManager;
std::shared_ptr<AudioDevice> AudioIO::audioDevice;
std::mutex AudioIO::init_mutex;

AudioIO::AudioIO() {
    /* Double-checking locking pattern from
      A Relaxed Guide to memory_order_relaxed - Paul E. McKenney & Hans Boehm - CppCon 2020 */
    if (refcount.load(std::memory_order_acquire) == 0) {
        std::lock_guard<std::mutex> _(init_mutex);
        if (refcount.load(std::memory_order_relaxed) == 0) {
            initAudioIO();
            refcount.fetch_add(1, std::memory_order_release);
        }
    } else {
        refcount.fetch_add(1, std::memory_order_relaxed);
    }
}

AudioIO::~AudioIO() {
    if (refcount.load(std::memory_order_seq_cst) == 1) {
        std::lock_guard<std::mutex> _(init_mutex);
        if (refcount.load(std::memory_order_relaxed) == 1) {
            destoryAudioIO();
            refcount.fetch_sub(1, std::memory_order_release);
        }
    } else {
        refcount.fetch_sub(1, std::memory_order_relaxed);
    }
}

void AudioIO::SendData(const uint8_t *data, int len) {
    jassert(len <= Config::MACDATA_PER_FRAME);

    MACManager::get().macTransmitter->SendPacket(data, len);
}

int AudioIO::RecvData(uint8_t *out, int outlen) {
    jassert(outlen <= Config::MACDATA_PER_FRAME);

    return MACManager::get().macReceiver->RecvPacket(out);
}

void AudioIO::initAudioIO() {
#ifdef DEBUG
    if (refcount != 0) {
        std::cout << "Error: AudioIO has been initialized\n";
        return;
    }
#endif
    audioDeviceManager.initialiseWithDefaultDevices(1, 1);
    AudioDeviceManager::AudioDeviceSetup dev_info = audioDeviceManager.getAudioDeviceSetup();
    dev_info.sampleRate = 48000;
    dev_info.bufferSize = 128;
    audioDevice = std::make_shared<AudioDevice>(Config::STATE);
    audioDeviceManager.addAudioCallback(audioDevice.get());

    std::unique_ptr<MACLayerReceiver> macReceiver;
    std::unique_ptr<MACLayerTransmitter> macTransmitter;

    std::cout << "AudioIO: selected mode: " << Config::STATE << "\n";

    macReceiver = std::make_unique<MACLayerReceiver>();
    macTransmitter = std::make_unique<MACLayerTransmitter>(audioDevice);

    MACManager::initialize(std::move(macReceiver), std::move(macTransmitter));
    audioDevice->beginTransmit();
}

void AudioIO::destoryAudioIO() {
#ifdef DEBUG
    if (refcount != 0) {
        std::cout << "Error: AudioIO is still referenced\n";
        return;
    }
#endif
    MACManager::destroy();
    audioDeviceManager.removeAudioCallback(audioDevice.get());
    audioDevice.reset();
}
