#pragma once

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <JuceHeader.h>
#include <condition_variable>
#include <mutex>
#include <list>

#include "Utils/Ringbuffer.hpp"
#include "Physical/FrameDetector.h"
#include "Physical/AudioFrame.h"
#include "Physical/Frame.h"
#include "Physical/Codec.h"
#include "MAC/MACLayer.h"

#define PI acos(-1)

using namespace juce;

typedef AudioBuffer<float> AudioType;

class AudioDevice;

class AudioIO {
public:
    AudioIO();

    ~AudioIO();

    // wrapper for macTransmitter
    static void SendData(const uint8_t *data, int len);

    // wrapper for macReceiver
    static int RecvData(uint8_t *out, int outlen);

private:

    static void initAudioIO();
    static void destoryAudioIO();

    static AudioDeviceManager audioDeviceManager;
    static std::shared_ptr<AudioDevice> audioDevice;

    static std::atomic<int> refcount;
    static std::mutex init_mutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioIO)
};


class AudioDevice : public AudioIODeviceCallback,
                    private HighResolutionTimer {
public:
    enum ChannelState {
        CN_IDLE,
        CN_BUSY,
    };

    explicit AudioDevice(enum state s);

    ~AudioDevice() override = default;

    void beginTransmit();

    void hiResTimerCallback() override;

    void sendFrame(const AudioFrame &frame);

    void audioDeviceAboutToStart(AudioIODevice *device) override;

    void audioDeviceStopped() override;

    void audioDeviceIOCallback(const float **inputChannelData, int numInputChannels,
                               float **outputChannelData, int numOutputChannels, int numSamples) override;

    static Codec codec;

    enum ChannelState getChannelState();

private:
    CriticalSection lock;
    RingBuffer<float> sender, receiver;
    std::list<Frame> received;

    FrameDetector frameDetector;

    enum state deviceState = Config::STATE;

    std::atomic<float> m_avgPower = 0;

    bool isSending = false;
    bool isReceiving = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDevice)
};

#endif
