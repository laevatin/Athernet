#pragma once

#ifndef __MACLAYER_H__
#define __MACLAYER_H__

#include <thread>
#include <list>
#include <condition_variable>
#include <atomic>
#include "Config.h"

class MACLayer
{
public:
    explicit MACLayer(std::shared_ptr<AudioDevice> audioDevice);
    virtual ~MACLayer();

    void stopMACThread();

protected: 
    std::thread *MACThread;
    std::atomic<bool> running;
    std::shared_ptr<AudioDevice> audioDevice;
};

class MACLayerReceiver : public MACLayer
{
public:
    explicit MACLayerReceiver(std::shared_ptr<AudioDevice> audioDevice);
    ~MACLayerReceiver();

private:
    void MACThreadRecvStart();
    
    std::mutex cv_m;
    std::condition_variable cv;

    enum RxState {
        IDLE,
        CK_HEADER,
        CK_FRAME,
        SEND_ACK
    };

    std::atomic<RxState> rxstate;
};


class MACLayerTransmitter : public MACLayer
{
public:
    explicit MACLayerTransmitter(const DataType &input, std::shared_ptr<AudioDevice> audioDevice);
    ~MACLayerTransmitter();

private:
    void MACThreadTransStart();

    std::mutex cv_m;
    std::condition_variable cv;

    enum TxState {
        IDLE,
        CK_HEADER,
        CK_ACK,
        SEND_DATA
    };

    std::atomic<TxState> txstate;
    DataType inputData;
};

#endif
