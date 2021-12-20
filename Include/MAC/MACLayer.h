#pragma once

#ifndef __MACLAYER_H__
#define __MACLAYER_H__

#include <thread>
#include <list>
#include <set>
#include <condition_variable>
#include <atomic>
#include <stdint.h>
#include "MAC/MACFrame.h"
#include "Physical/Frame.h"

class AudioDevice;

class CSMASenderQueue;

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

    std::mutex m_mQueue;
    std::condition_variable m_cvQueue;
    std::list<Frame> m_recvQueue;

    std::mutex m_mPacketQueue;
    std::condition_variable m_cvPacketQueue;
    std::list<MACFrame> m_packetQueue;
};


class MACLayerTransmitter : public MACLayer {
public:
    explicit MACLayerTransmitter(std::shared_ptr<AudioDevice> audioDevice);

    ~MACLayerTransmitter() override;

    /* This function may be called from other thread. */
    void ReplyReceived(MACFrame &reply);

    void SendPacket(const uint8_t *data, int len);

    void SendPing(MACType pingType);

    void SendACK(uint8_t id);

private:
    void MACThreadTransStart();

    enum TxState {
        IDLE,
        ACK_RECEIVED,
        PING_RECEIVED,
        SEND_DATA,
        SEND_PING
    };

    std::unique_ptr<CSMASenderQueue> m_asyncSender;

    std::mutex m_mAck;
    std::condition_variable m_cvAck;

    std::mutex m_mSend;
    std::condition_variable m_cvSend;

    std::list<MACFrame> m_sendQueue;
    std::map<uint8_t, MACFrame> m_sentWindow;
    std::atomic<TxState> m_txState;
    std::atomic<uint8_t> m_lastId;
};

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
    std::mutex m_mQueue;
    std::condition_variable m_cvQueue;
    std::list<std::pair<uint16_t, AudioFrame>> m_queue;

    std::atomic<bool> running;
};

#endif
