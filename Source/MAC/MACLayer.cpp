#include "MAC/MACLayer.h"
#include "MAC/Serde.h"
#include "Physical/Audio.h"
#include "MAC/MACManager.h"

#include <chrono>
#include <memory>
#include <utility>

#include <windows.h>

using namespace std::chrono_literals;

MACLayer::MACLayer()
    : running(true)
{}

void MACLayer::stopMACThread()
{
    running = false;
}


MACLayerReceiver::MACLayerReceiver()
    : rxstate(MACLayerReceiver::IDLE)
{
    MACThread = std::make_unique<std::thread>([this]() { MACThreadRecvStart(); });
}

MACLayerReceiver::~MACLayerReceiver()
{
    if (MACThread)
    {
        cv_frameQueue.notify_one();
        MACThread->join();
    }
}

void MACLayerReceiver::MACThreadRecvStart()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_frameQueue);

        cv_frameQueue.wait(lock, [this]() { return !frameQueue.empty() || !running; });

        if (!running)
            break;

        MACFrame macFrame;
        if (frameQueue.front().isGoodFrame())
        {
            convertMACFrame(frameQueue.front(), &macFrame);
            sendACK(macFrame.header.id);
            packetQueue.push_back(macFrame);
            cv_packetQueue.notify_one();
        }
        frameQueue.pop_front();
    }
}

void MACLayerReceiver::frameReceived(Frame &&frame)
{
    m_frameQueue.lock();
    frameQueue.push_back(std::move(frame));
    m_frameQueue.unlock();

    cv_frameQueue.notify_one();
}

bool MACLayerReceiver::checkFrame(const MACHeader *macHeader)
{
    bool success = (macHeader->src  == Config::OTHER)
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
    frameQueue.emplace_back();
}

int MACLayerReceiver::RecvPacket(uint8_t *out)
{
    std::unique_lock<std::mutex> lock_packet(m_packetQueue);
    cv_packetQueue.wait(lock_packet, [this]() { return !packetQueue.empty(); });

    int len = packetQueue.front().header.len;
    uint8_t *frame = serialize(&packetQueue.front());
    memcpy(out, frame + sizeof(MACHeader), Config::MACDATA_PER_FRAME);
    packetQueue.pop_front();
    return len;
}


MACLayerTransmitter::MACLayerTransmitter()
    : txstate(MACLayerTransmitter::IDLE)
{
    MACThread = std::make_unique<std::thread>([this]() { MACThreadTransStart(); });
}

MACLayerTransmitter::~MACLayerTransmitter()
{
    if (MACThread)
    {
        cv_queue.notify_one();
        MACThread->join();
    }
}

void MACLayerTransmitter::ACKReceived(Frame&& ack)
{
    MACFrame macFrame;
    convertMACFrame(ack, &macFrame);

    if (macFrame.header.type == Config::MACPING_REPLY && txstate == SEND_PING)
    {
        txstate = PING_RECEIVED;
        cv_ack.notify_one();
        return;
    }
    
    if (txstate != SEND_PING && macFrame.header.id == pendingFrame.front().id()
        && MACManager::get().macFrameFactory->checkCRC(&macFrame)) 
    {
        txstate = ACK_RECEIVED;
        cv_ack.notify_one();
        return;
    }

    std::cout << "SENDER: Received bad ACK " << (int)macFrame.header.id << " type " << (int)macFrame.header.type << ".\n";
}

bool MACLayerTransmitter::checkACK(const MACHeader *macHeader)
{
    bool success = (macHeader->src  == Config::OTHER)
            && (macHeader->dest == Config::SELF)
            && (macHeader->type == Config::ACK);
    return success;
}

bool MACLayerTransmitter::checkPingReq(const MACHeader* macHeader) {
	bool success = (macHeader->src == Config::OTHER)
		&& (macHeader->dest == Config::SELF)
		&& (macHeader->type == Config::MACPING_REQ)
        && (macHeader->id == Config::MACPING_ID);
	return success;
}

bool MACLayerTransmitter::checkPingReply(const MACHeader* macHeader) {
	bool success = (macHeader->src == Config::OTHER)
		&& (macHeader->dest == Config::SELF)
		&& (macHeader->type == Config::MACPING_REPLY)
        && (macHeader->id == Config::MACPING_ID);
	return success;
}

void MACLayerTransmitter::MACThreadTransStart()
{
    int resendCount = 0;
    while (true)
    {
        std::unique_lock<std::mutex> lock_queue(m_queue);
        cv_queue.wait(lock_queue, [this]() { return !pendingFrame.empty() || !running; });

        if (!running)
            break;

        txstate = SEND_DATA;
        MACManager::get().csmaSenderQueue->sendDataAsync(Frame(pendingFrame.front()));
        lock_queue.unlock();

        auto now = std::chrono::system_clock::now();
        std::unique_lock<std::mutex> lock(cv_ack_m);
        if (cv_ack.wait_until(lock, now + Config::ACK_TIMEOUT, [this](){ return txstate == ACK_RECEIVED; }))
        {
            resendCount = 0;
            auto recv = std::chrono::system_clock::now();
#ifdef VERBOSE_MAC
            std::cout << "SENDER: Time to ACK " << (int)pendingFrame.front().id() << ": "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() << "\n";
#endif
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
            std::cout << "SENDER: Get Ping After: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() << " milliseconds.\n";
        }
        Sleep(400);
    }
}

void MACLayerTransmitter::SendPacket(const uint8_t *data, int len)
{
    jassert(len <= Config::MACDATA_PER_FRAME);

    MACFrame *macFrame = MACManager::get().macFrameFactory->createDataFrame(data, 0, len);
    pendingFrame.push_back(convertFrame(macFrame));
    cv_queue.notify_one();
    MACManager::get().macFrameFactory->destoryFrame(macFrame);
}

CSMASenderQueue::CSMASenderQueue(std::shared_ptr<AudioDevice> audioDevice)
    : m_audioDevice(std::move(audioDevice))
{
    m_senderThread = std::make_unique<std::thread>([this]() { senderStart(); });
}

CSMASenderQueue::~CSMASenderQueue()
{
    running = false;
    if (m_senderThread != nullptr)
    {
        m_cv_frame.notify_one();
        m_senderThread->join();
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
        MACFrame *macFrame = MACManager::get().macFrameFactory->createACKFrame(id);
        m_queue_m.lock();
        m_hasACKid[id] = true;
        m_queue.push_back(convertFrame(macFrame));
        m_queue_m.unlock();
        m_cv_frame.notify_one();
        MACManager::get().macFrameFactory->destoryFrame(macFrame);
    }
}

void CSMASenderQueue::sendPingAsync(uint8_t id, uint8_t type)
{
    MACFrame *macFrame = nullptr;
    if (type == Config::MACPING_REQ)
        macFrame = MACManager::get().macFrameFactory->createPingReq(id);
    else if (type == Config::MACPING_REPLY)
        macFrame = MACManager::get().macFrameFactory->createPingReply(id);
    else  // unknown type
        return;

    m_queue_m.lock();
    m_hasACKid[id] = true;
    m_queue.push_back(convertFrame(macFrame));
    m_queue_m.unlock();
    m_cv_frame.notify_one();
    MACManager::get().macFrameFactory->destoryFrame(macFrame);
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

            if (m_audioDevice->getChannelState() == AudioDevice::CN_BUSY)
                Sleep(10);

            if (m_audioDevice->getChannelState() == AudioDevice::CN_IDLE) 
            {
                m_audioDevice->sendFrame(m_queue.front());
                if (m_queue.front().isACK()) {
                    m_hasACKid[m_queue.front().id()] = false;
#ifdef VERBOSE_MAC
                    std::cout << "SENDER: Send ACK:  " << (int)m_queue.front().id() << "\n";
#endif
                }
                else
                {
#ifdef VERBOSE_MAC
                    std::cout << "SENDER: Send Frame: " << (int)m_queue.front().id() << "\n";
#endif
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