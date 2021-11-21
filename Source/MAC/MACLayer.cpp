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

        if (!cv_header.wait_until(lock, now + 20s, [this]() { return !receivingQueue.empty(); })) 
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

MACLayerTransmitter::MACLayerTransmitter(std::shared_ptr<AudioDevice> audioDevice)
    : MACLayer(audioDevice),
    txstate(MACLayerTransmitter::SEND_PING)
{
    MACThread = new std::thread([this]() { MACThreadPingStart(); });
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

    if (macFrame.header.type == Config::MACPING_REPLY && txstate == SEND_PING)
    {
        txstate = PING_RECEIVED;
        cv_ack.notify_one();
        return;
    }
    
    if (txstate != SEND_PING && macFrame.header.id == pendingFrame.front().id()) 
    {
        txstate = ACK_RECEIVED;
        cv_ack.notify_one();
        return;
    }

    std::cout << "SENDER: Received bad ACK " << (int)macFrame.header.id << " type " << (int)macFrame.header.type << ".\n";
}

bool MACLayerTransmitter::checkACK(const MACHeader *macHeader)
{
    bool success = true;
    success = success && (macHeader->src  == Config::OTHER)
                      && (macHeader->dest == Config::SELF)
                      && (macHeader->type == Config::ACK);
    return success;
}

bool MACLayerTransmitter::checkPingReq(const MACHeader* macHeader) {
	bool success = true;
	success = success && (macHeader->src == Config::OTHER)
		&& (macHeader->dest == Config::SELF)
		&& (macHeader->type == Config::MACPING_REQ)
        && (macHeader->id == Config::MACPING_ID);
	return success;
}

bool MACLayerTransmitter::checkPingReply(const MACHeader* macHeader) {
	bool success = true;
	success = success && (macHeader->src == Config::OTHER)
		&& (macHeader->dest == Config::SELF)
		&& (macHeader->type == Config::MACPING_REPLY)
        && (macHeader->id == Config::MACPING_ID);
	return success;
}

void MACLayerTransmitter::MACThreadTransStart()
{
    int resendCount = 0;
    fillQueue();
    auto time1 = std::chrono::system_clock::now();
    auto time2 = time1;

    int receivedCount = 0;
    int receivedSaved = 0;

    while (running && !pendingFrame.empty())
    {
        txstate = SEND_DATA;
        /* Send a copy */
        MACManager::get().csmaSenderQueue->sendDataAsync(Frame(pendingFrame.front()));

        auto now = std::chrono::system_clock::now();

        /* fillQueue should not take too much time. */
        fillQueue();
        time2 = std::chrono::system_clock::now();

        if (time2 - time1 > 1s)
        {
            time1 = std::chrono::system_clock::now();
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1).count();
            int frameSent = receivedCount - receivedSaved;
            receivedSaved = receivedCount;
            float speed = ((float)frameSent * (Config::MACDATA_PER_FRAME) * 8) / (float)time;
            std::cout << "SENDER: Current transfer speed: " << speed << " Kbps. \n";
        }
        std::unique_lock<std::mutex> lock(cv_ack_m);

        if (cv_ack.wait_until(lock, now + Config::ACK_TIMEOUT, [this](){ return txstate == ACK_RECEIVED; }))
        {
            resendCount = 0;
            receivedCount += 1;
            auto recv = std::chrono::system_clock::now();
            std::cout << "SENDER: Time to ACK " << (int)pendingFrame.front().id() << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() << "\n";
            pendingFrame.pop_front();
        }
        else
        {
            resendCount += 1;
            if (resendCount >= 10)
            {
                std::cout << "Link Error: no ACK received after 10 retries.\n";
                break;
            }
        }
    }
    audioDevice->stopSending();
}

void MACLayerTransmitter::MACThreadPingStart()
{
    while (running)
    {
        txstate = SEND_PING;
        auto now = std::chrono::system_clock::now();
        MACManager::get().csmaSenderQueue->sendPingAsync(Config::MACPING_ID, Config::MACPING_REQ);

        std::unique_lock<std::mutex> lock(cv_ack_m);
        if (cv_ack.wait_until(lock, now + 2s, [this](){ return txstate == PING_RECEIVED; }))
        {
            auto recv = std::chrono::system_clock::now();
            std::cout << "SENDER: Get Ping After: " << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() << " milliseconds.\n";
        }
        Sleep(1000);
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
        m_queue.push_back(convertFrame(macFrame));
        m_queue_m.unlock();
        m_cv_frame.notify_one();
        frameFactory.destoryFrame(macFrame);
    }
}

void CSMASenderQueue::sendPingAsync(uint8_t id, uint8_t type)
{
    MACFrame *macFrame;
    if (type == Config::MACPING_REQ)
        macFrame = frameFactory.createPingReq(id);
    if (type == Config::MACPING_REPLY)
        macFrame = frameFactory.createPingReply(id);

    m_queue_m.lock();
    m_hasACKid[id] = true;
    m_queue.push_back(convertFrame(macFrame));
    m_queue_m.unlock();
    m_cv_frame.notify_one();
    frameFactory.destoryFrame(macFrame);
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
            Sleep(Config::BACKOFF_TSLOT * (int)pow(2, count - 1));
            lock_queue.lock();
        }
    }
}