#pragma once

#ifndef __MACLAYER_H__
#define __MACLAYER_H__

#include <thread>
#include <list>
#include <condition_variable>
#include "Config.h"

class MACLayer
{
public:
    /* If input is provided, regard the thread as transmitter. */
    explicit MACLayer(const DataType &input);
    explicit MACLayer();

    ~MACLayer();

    void stopMACThread();

private: 
    std::thread* MACThread;
    DataType inputData;

    enum RxState {
        IDLE,
        CK_HEADER,
        CK_FRAME,
        SEND_ACK
    };

    enum TxState {
        IDLE,
        CK_HEADER,
        CK_ACK,
        SEND_DATA
    };

    volatile RxState rxstate;
    volatile TxState txstate;

    std::mutex cv_m;
    std::condition_variable cv;

    volatile bool running;

    void MACThreadTransStart();
    void MACThreadRecvStart();

};

#endif
