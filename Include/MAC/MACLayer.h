#pragma once

#ifndef __MACLAYER_H__
#define __MACLAYER_H__

#include <thread>
#include <list>
#include <condition_variable>
#include <atomic>
#include <stdint.h>
#include "MAC/MACFrame.h"
#include "Physical/Frame.h"

class AudioDevice;

class MACLayer
{
public:
    explicit MACLayer(std::shared_ptr<AudioDevice> audioDevice);
    virtual ~MACLayer() = 0;

    virtual void stopMACThread();

protected:
    std::thread *MACThread;
    std::atomic<bool> running;
    std::shared_ptr<AudioDevice> audioDevice;
};

class MACLayerReceiver : public MACLayer
{
public:
    explicit MACLayerReceiver(std::shared_ptr<AudioDevice> audioDevice);
    ~MACLayerReceiver() override;

    void frameReceived(Frame &&frame);
    void stopMACThread() override;
    static static void sendACK(uint8_t id);
    int RecvPacket(uint8_t *out);

    static bool checkFrame(const MACHeader *macHeader);

private:
    void MACThreadRecvStart();
    
    enum RxState {
        IDLE,
        CK_HEADER,
        CK_FRAME,
        SEND_ACK
    };

    std::mutex m_frameQueue;
    std::condition_variable cv_frameQueue;
    std::list<Frame> frameQueue;

    std::mutex m_packetQueue;
    std::condition_variable cv_packetQueue;
    std::list<MACFrame> packetQueue;

    std::atomic<RxState> rxstate;
};


class MACLayerTransmitter : public MACLayer
{
public:
    // send data
    explicit MACLayerTransmitter(std::shared_ptr<AudioDevice> audioDevice);
    
    // send ping
    // explicit MACLayerTransmitter(std::shared_ptr<AudioDevice> audioDevice);
    
    ~MACLayerTransmitter() override;

    /* This function may called from other thread. */
    void ACKReceived(Frame&& ack);
    void SendPacket(const uint8_t *data, int len);

    static bool checkACK(const MACHeader *macHeader);

	static bool checkPingReq(const MACHeader* macHeader);

	static bool checkPingReply(const MACHeader* macHeader);
private:
    void MACThreadTransStart();
    void MACThreadPingStart();

    enum TxState {
        IDLE,
        ACK_RECEIVED,
        PING_RECEIVED,
        SEND_DATA,
        SEND_PING
    };
    
    std::mutex cv_ack_m;
    std::condition_variable cv_ack;
    
    std::mutex m_queue;
    std::condition_variable cv_queue;

    std::list<Frame> pendingFrame;
    std::list<uint8_t> pendingID;

    std::atomic<TxState> txstate;
};

class CSMASenderQueue
{
public: 
    explicit CSMASenderQueue(std::shared_ptr<AudioDevice> audioDevice);
    ~CSMASenderQueue();

    void sendDataAsync(Frame&& frame);
    void sendACKAsync(uint8_t id);
    void sendPingAsync(uint8_t id, uint8_t type);

private:
    void senderStart();
    
    std::thread *m_senderThread;
    std::shared_ptr<AudioDevice> m_audioDevice;

    std::mutex m_queue_m;
    std::list<Frame> m_queue;
    std::condition_variable m_cv_frame;

    bool m_hasACKid[256] = { false };
    bool m_hasDATAid[256] = { false };
    std::atomic<bool> running = true;

};

#endif
