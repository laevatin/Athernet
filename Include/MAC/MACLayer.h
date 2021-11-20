#pragma once

#ifndef __MACLAYER_H__
#define __MACLAYER_H__

#include <thread>
#include <list>
#include <condition_variable>
#include <atomic>
#include "MAC/MACFrame.h"
#include "Physical/Frame.h"

class AudioDevice;

class MACLayer
{
public:
    explicit MACLayer(std::shared_ptr<AudioDevice> audioDevice);
    virtual ~MACLayer();

    virtual void stopMACThread();

protected:
    MACFrameFactory frameFactory;
    std::thread *MACThread;
    std::atomic<bool> running;
    std::shared_ptr<AudioDevice> audioDevice;
};

class MACLayerReceiver : public MACLayer
{
public:
    explicit MACLayerReceiver(std::shared_ptr<AudioDevice> audioDevice);
    ~MACLayerReceiver();

    void frameReceived(Frame &&frame);
    void stopMACThread() override;
    void getOutput(DataType &out);
    void sendACK(uint8_t id);
    static bool checkFrame(const MACHeader *macHeader);

private:
    void MACThreadRecvStart();
    
    enum RxState {
        IDLE,
        CK_HEADER,
        CK_FRAME,
        SEND_ACK
    };

    std::mutex cv_header_m;
    std::condition_variable cv_header;

    DataType outputData;
    std::list<Frame> receivingQueue;
    std::atomic<RxState> rxstate;
};


class MACLayerTransmitter : public MACLayer
{
public:
    explicit MACLayerTransmitter(const DataType &input, std::shared_ptr<AudioDevice> audioDevice);
    ~MACLayerTransmitter();

    /* This function may called from other thread. */
    void ACKReceived(const Frame &ack);

    static bool checkACK(const MACHeader *macHeader);
private:
    void MACThreadTransStart();
    void fillQueue();

    enum TxState {
        IDLE,
        CK_HEADER,
        ACK_RECEIVED,
        SEND_DATA
    };
    
    std::mutex cv_ack_m;
    std::condition_variable cv_ack;

    std::list<Frame> pendingFrame;
    std::list<uint8_t> pendingID;

    int currentID;
    std::atomic<TxState> txstate;

    DataType inputData;
    int inputPos;
};

class CSMASenderQueue
{
public: 
    explicit CSMASenderQueue(std::shared_ptr<AudioDevice> audioDevice);
    ~CSMASenderQueue();

    void sendDataAsync(Frame&& frame);
    void sendACKAsync(uint8_t id);

private:
    void senderStart();
    
    std::thread *m_senderThread;
    std::shared_ptr<AudioDevice> m_audioDevice;

    std::mutex m_queue_m;
    std::list<Frame> m_queue;
    std::condition_variable m_cv_frame;

    bool m_hasACKid[256];
    bool m_hasDATAid[256];
    bool running = true;

    MACFrameFactory frameFactory;
};

#endif
