#include "MAC/MACLayer.h"
#include "MAC/Serde.h"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

MACLayer::MACLayer(std::shared_ptr<AudioDevice> audioDevice)
    : running(true),
    audioDevice(audioDevice)
{}

MACLayer::~MACLayer()
{}

void MACLayer::stopMACThread()
{
    running = false;
}

// ----------------------------------------------------------------------------

MACLayerReceiver::MACLayerReceiver(std::shared_ptr<AudioDevice> audioDevice)
    : MACLayer(audioDevice),
    rxstate(MACLayerReceiver::IDLE)
{
    MACThread = new std::thread([this]() { MACThreadRecvStart(); });
}

MACLayerReceiver::~MACLayerReceiver()
{
    if (MACThread != nullptr)
    {
        MACThread->join();
        delete MACThread;
    }
}

void MACLayerReceiver::MACThreadRecvStart()
{
    while (running)
    {
               
    }
}

// ----------------------------------------------------------------------------

MACLayerTransmitter::MACLayerTransmitter(const DataType &input, std::shared_ptr<AudioDevice> audioDevice)
    : MACLayer(audioDevice),
    txstate(MACLayerTransmitter::IDLE)
{
    inputData = input;
    MACThread = new std::thread([this]() { MACThreadTransStart(); });
}

MACLayerTransmitter::~MACLayerTransmitter()
{
    if (MACThread != nullptr)
    {
        MACThread->join();
        delete MACThread;
    }
}

void MACLayerTransmitter::MACThreadTransStart()
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
