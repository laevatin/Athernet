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
        /**
         * 1. block on checkHeader (hiResTimerCallback)
         * 2. if (Bad Frame)
         *      continue;
         * 3. sendACK(frame->id)
         **/

        std::unique_lock<std::mutex> lock(cv_header_m);

        cv_header.wait(lock, receivingQueue.empty());

        MACFrame *macFrame = frameFactory.createEmpty();
        convertMACFrame(receivingQueue.front(), macFrame);
        receivingQueue.pop_front();
        lock.unlock();

        if (!checkFrame(macFrame))
            continue;

        sendACK(macFrame->id);

        // get data from macFrame
    }
}

bool MACLayerReceiver::checkFrame(const MACFrame *macFrame)
{
    bool success = true;
    success = success && (macFrame->src  == Config::SENDER)
                      && (macFrame->dest == Config::RECEIVER)
                      && (macFrame->type == Config::DATA);
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
    MACFrame *macFrame = frameFactory.createEmpty();
    convertMACFrame(ack, macFrame);

    if (!checkACK(macFrame))
    {
        txstate = ACK_RECEIVED;
        cv_ack.notify_one();
    }

    frameFactory.destoryFrame(macFrame);
}

bool MACLayerTransmitter::checkACK(const MACFrame *macFrame)
{
    bool success = true;
    success = success && (macFrame->src  == Config::RECEIVER)
                      && (macFrame->dest == Config::SENDER)
                      && (macFrame->type == Config::ACK);
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
            pendingFrame.pop_back();
            pendingID.pop_back();
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
        pendingFrame.push_back(convertFrame(macFrame));
        pendingID.push_back(macFrame->id);
        frameFactory.destoryFrame(macFrame);
    }
}
