#include "MAC/MACLayer.h"
#include "MAC/Serde.h"
#include "Physical/Audio.h"
#include "Utils/IOFunctions.hpp"
#include "MAC/MACManager.h"

#include <chrono>
#include <memory>

#include <windows.h>

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
        std::unique_lock<std::mutex> lock(cv_header_m);
        auto now = std::chrono::system_clock::now();

        if (!cv_header.wait_until(lock, now + 5s, [this]() { return !receivingQueue.empty(); })) 
        {
            audioDevice->stopReceiving();
            audioDevice->stopSending();
            break;
        }

        MACFrame macFrame;
        if (receivingQueue.front().isGoodFrame())
        {
            convertMACFrame(receivingQueue.front(), &macFrame);
            sendACK(macFrame.header.id);
            outputData.addArray(macFrame.data, macFrame.header.len);
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
    success = success && (macHeader->src  == Config::OTHER)
                      && (macHeader->dest == Config::SELF)
                      && (macHeader->type == Config::DATA);
    return success;
}

void MACLayerReceiver::sendACK(uint8_t id)
{
    MACManager::get().csmaSenderQueue->sendACKAsync(id);
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

void MACLayerTransmitter::ACKReceived(const Frame &ack)
{
    MACFrame macFrame;
    convertMACFrame(ack, &macFrame);

    if (macFrame.header.id == pendingFrame.front().id()) 
    {
        txstate = ACK_RECEIVED;
        cv_ack.notify_one();
        return;
    }

    std::cout << "SENDER: Received bad ACK " << (int)macFrame.header.id << "current" << (int)pendingFrame.front().id() << ".\n";
}

bool MACLayerTransmitter::checkACK(const MACHeader *macHeader)
{
    bool success = true;
    success = success && (macHeader->src  == Config::OTHER)
                      && (macHeader->dest == Config::SELF)
                      && (macHeader->type == Config::ACK);
    return success;
}

void MACLayerTransmitter::MACThreadTransStart()
{
    fillQueue();
    while (running && !pendingFrame.empty())
    {
        txstate = SEND_DATA;
        /* Send a copy */
        MACManager::get().csmaSenderQueue->sendDataAsync(Frame(pendingFrame.front()));

        auto now = std::chrono::system_clock::now();

        /* fillQueue should not take too much time. */
        fillQueue();
        std::unique_lock<std::mutex> lock(cv_ack_m);

        if (cv_ack.wait_until(lock, now + Config::ACK_TIMEOUT, [this](){ return txstate == ACK_RECEIVED; }))
        {
            auto recv = std::chrono::system_clock::now();
            std::cout << "SENDER: Time to ACK " << (int)pendingFrame.front().id() << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() << "\n";
            pendingFrame.pop_front();
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
        frameFactory.destoryFrame(macFrame);
    }
}

CSMASenderQueue::CSMASenderQueue(std::shared_ptr<AudioDevice> audioDevice)
    : m_audioDevice(audioDevice)
{
    m_senderThread = new std::thread([this]() { senderStart(); });
}

CSMASenderQueue::~CSMASenderQueue()
{
    running = false;
    if (m_senderThread != nullptr)
    {
        m_cv_frame.notify_one();
        m_senderThread->join();
        delete m_senderThread;
    }
}

void CSMASenderQueue::sendDataAsync(Frame&& frame)
{
    if (m_hasDATAid[frame.id()])
        return;
    else 
    {
        m_queue_m.lock();
        m_hasDATAid[frame.id()] = true;
        m_queue.push_back(std::move(frame));
        m_queue_m.unlock();
        m_cv_frame.notify_one();
    }
}

void CSMASenderQueue::sendACKAsync(uint8_t id)
{
    if (m_hasACKid[id])
        return;
    else 
    {
        MACFrame *macFrame = frameFactory.createACKFrame(id);
        m_queue_m.lock();
        m_hasACKid[id] = true;
        m_queue.push_front(convertFrame(macFrame));
        m_queue_m.unlock();
        m_cv_frame.notify_one();
        frameFactory.destoryFrame(macFrame);
    }
}

void CSMASenderQueue::senderStart()
{
    while (running) 
    {
        std::unique_lock<std::mutex> lock_queue(m_queue_m);
        m_cv_frame.wait(lock_queue, [this]() { return !m_queue.empty() || !running; } );

        int count = 0;
        while (!m_queue.empty() && running) 
        {
            count++;

            if (m_audioDevice->getChannelState() == AudioDevice::CN_IDLE) 
            {
                
                m_audioDevice->sendFrame(m_queue.front());
                if (m_queue.front().isACK()) {
                    m_hasACKid[m_queue.front().id()] = false;
                    std::cout << "SENDER: Send ACK:  " << (int)m_queue.front().id() << "\n";
                }
                else{
                    std::cout << "SENDER: Send Frame: " << (int)m_queue.front().id() << "\n";
                    m_hasDATAid[m_queue.front().id()] = false;
                } 
                    
                m_queue.pop_front();                
                break;
            }

            lock_queue.unlock();
            std::cout << "Channel busy, retrying " << count << "\n";
            if (count >= 10) 
                count = 10;
            Sleep(Config::BACKOFF_TSLOT * (int)pow(2, count));
            lock_queue.lock();
        }
    }
}