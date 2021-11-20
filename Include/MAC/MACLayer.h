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

#endif
