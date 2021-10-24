#include "MAC/MACLayer.h"
#include <chrono>

using namespace std::chrono_literals;

MACLayer::MACLayer(const DataType &input)
    : rxstate(RxState::IDLE),
    txstate(TxState::IDLE),
    running(true)
{
    inputData = input;
    MACThread = new std::thread([this](){ MACThreadTransStart(); });
}

MACLayer::MACLayer()
    : rxstate(RxState::IDLE),
    txstate(TxState::IDLE),
    running(true)
{
    MACThread = new std::thread([this](){ MACThreadRecvStart(); });
}

MACLayer::~MACLayer()
{
    if (MACThread != nullptr)
    {
        MACThread->join();
        delete MACThread;
    }
}

void MACLayer::MACThreadTransStart()
{
    while (running)
    {
        // sendframe
        // if queue not full && data remaining
        //   createMACFrame
        //   serialization
        //   put into queue
        // if (cv.wait_until(now+100ms, send_data))
        //   popframe (send next frame)
    }
    
}

void MACLayer::MACThreadRecvStart()
{
    while (running)
    {
        // sendframe
        // if (cv.wait_until(now+100ms, send_data))
        //           
    }
}

void MACLayer::stopMACThread()
{
    running = false;
}

// MACLayer::

