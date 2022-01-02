#pragma once

#ifndef __MACLAYER_H__
#define __MACLAYER_H__

#include <thread>
#include <list>
#include <set>
#include <condition_variable>
#include <atomic>
#include "MAC/MACFrame.h"
#include "MAC/SlidingWindow.h"
#include "Physical/Frame.h"
#include "Utils/BlockingQueue.h"

class AudioDevice;

class CSMASenderQueue {
public:
    explicit CSMASenderQueue(std::shared_ptr<AudioDevice> audioDevice);

    ~CSMASenderQueue();

    void SendMACFrameAsync(const MACFrame &macFrame);

    void SendACKAsync(uint8_t id);

private:
    void SenderStart();

    std::unique_ptr<std::thread> m_senderThread;
    std::shared_ptr<AudioDevice> m_audioDevice;

    /* low probability to crc collide */
    std::set<uint16_t> m_hasFrame;
    BlockingQueue<std::pair<uint16_t, AudioFrame>> m_queue;

    std::atomic<bool> running;
};

class MACLayer {
public:
    explicit MACLayer();

    virtual ~MACLayer() = default;

protected:
    std::unique_ptr<std::thread> MACThread;
    std::atomic<bool> running;
};

class MACLayerReceiver : public MACLayer {
public:
    explicit MACLayerReceiver();

    ~MACLayerReceiver() override;

    /* For frame detector */
    void frameReceived(Frame &&frame);

    int RecvPacket(uint8_t *out);

private:
    void MACThreadRecvStart();

    BlockingQueue<Frame> m_recvQueue;
    BlockingQueue<MACFrame> m_packetQueue;

    ReorderQueue m_reorderQueue;
};


class MACLayerTransmitter : public MACLayer {
public:
    explicit MACLayerTransmitter(std::shared_ptr<AudioDevice> audioDevice);

    ~MACLayerTransmitter() override;

    /* This function may be called from other thread. */
    void ReplyReceived(MACFrame &reply);

    void SendPacket(const uint8_t *data, int len);

    void SendACK(uint8_t id);

private:
    void MACThreadTransStart();

    void SlidingWindowSender(int windowIdx);

    CSMASenderQueue m_asyncSender;

    std::mutex m_mAck;
    std::condition_variable m_cvAck;

    BlockingQueue<MACFrame> m_sendQueue;

    SlidingWindow m_slidingWindow;
};

#endif
