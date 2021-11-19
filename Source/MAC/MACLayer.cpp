#include "MAC/MACLayer.h"
#include "MAC/Serde.h"
#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"

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
        /**
         * 1. block on checkHeader (hiResTimerCallback)
         * 2. if (Bad Frame)
         *      continue;
         * 3. sendACK(frame->id)
         **/

        std::unique_lock<std::mutex> lock(cv_header_m);

        cv_header.wait(lock, [this](){return !receivingQueue.empty();});

        MACFrame macFrame;
        convertMACFrame(receivingQueue.front(), &macFrame);
        if (receivingQueue.front().isGoodFrame())
        {
            sendACK(macFrame.header.id);
            outputData.addArray(macFrame.data, macFrame.header.len);
            if (macFrame.header.len < Config::MACDATA_PER_FRAME)
            {
                audioDevice->stopReceiving();
                audioDevice->stopSending();
            }
        }
        receivingQueue.pop_front();
    }
}

void MACLayerReceiver::frameReceived(Frame &&frame)
{
    cv_header_m.lock();
    receivingQueue.push_back(std::move(frame));
    cv_header_m.unlock();

    cv_header.notify_one();
}

bool MACLayerReceiver::checkFrame(const MACHeader *macHeader)
{
    bool success = true;
    success = success && (macHeader->src  == Config::SENDER)
                      && (macHeader->dest == Config::RECEIVER)
                      && (macHeader->type == Config::DATA);
    return success;
}

void MACLayerReceiver::sendACK(uint8_t id)
{
    MACFrame *macFrame = frameFactory.createACKFrame(id);
    audioDevice->sendFrame(convertFrame(macFrame));
    frameFactory.destoryFrame(macFrame);
}

void MACLayerReceiver::stopMACThread()
{
    running = false;
    /* Add an empty frame to wake the thread up. */
    receivingQueue.emplace_back();
}

void MACLayerReceiver::getOutput(DataType &out)
{
    out.addArray(outputData);
}

// ----------------------------------------------------------------------------

MACLayerTransmitter::MACLayerTransmitter(const DataType &input, std::shared_ptr<AudioDevice> audioDevice)
    : MACLayer(audioDevice),
    txstate(MACLayerTransmitter::IDLE),
    inputPos(0)
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

void MACLayerTransmitter::ACKreceived(const Frame &ack)
{
    MACFrame macFrame;
    convertMACFrame(ack, &macFrame);

    if (checkACK(&macFrame.header))
    {
        txstate = ACK_RECEIVED;
        cv_ack.notify_one();
    }
}

bool MACLayerTransmitter::checkACK(const MACHeader *macHeader)
{
    bool success = true;
    success = success && (macHeader->src  == Config::RECEIVER)
                      && (macHeader->dest == Config::SENDER)
                      && (macHeader->type == Config::ACK);
    return success;
}

void MACLayerTransmitter::MACThreadTransStart()
{
    fillQueue();
    while (running && !pendingFrame.empty())
    {
        txstate = SEND_DATA;
        audioDevice->sendFrame(pendingFrame.front());

        auto now = std::chrono::system_clock::now();

        /* fillQueue should not take too much time. */
        fillQueue();
        std::unique_lock<std::mutex> lock(cv_ack_m);

        if (cv_ack.wait_until(lock, now + Config::ACK_TIMEOUT, [this](){ return txstate == ACK_RECEIVED; }))
        {
            auto recv = std::chrono::system_clock::now();
            std::cout << "Time to ACK: " << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() << "\n";
            pendingFrame.pop_front();
            pendingID.pop_front();
        }
    }
}

void MACLayerTransmitter::fillQueue()
{
    while (pendingFrame.size() < Config::PENDING_QUEUE_SIZE && inputPos < inputData.size())
    {
        int length = Config::MACDATA_PER_FRAME;
        if (inputPos + length > inputData.size())
            length = inputData.size() - inputPos;

        MACFrame *macFrame = frameFactory.createDataFrame(inputData, inputPos, length);
        inputPos += length;
        pendingFrame.push_back(convertFrame(macFrame));
        pendingID.push_back(macFrame->header.id);
        frameFactory.destoryFrame(macFrame);
    }
}
