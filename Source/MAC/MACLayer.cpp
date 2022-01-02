#include "MAC/MACLayer.h"
#include "Physical/Audio.h"
#include "MAC/MACManager.h"

#include <chrono>
#include <memory>
#include <utility>
#include <future>

using namespace std::chrono_literals;

MACLayer::MACLayer()
        : running(true) {}

MACLayerReceiver::MACLayerReceiver()
        : m_reorderQueue(Config::SLIDING_WINDOW_SIZE) {
    MACThread = std::make_unique<std::thread>([this]() { MACThreadRecvStart(); });
}

MACLayerReceiver::~MACLayerReceiver() {
    running = false;
    if (MACThread) {
        m_recvQueue.push_back(Frame());
        MACThread->join();
    }
}

void MACLayerReceiver::MACThreadRecvStart() {
    while (running) {
        if (m_recvQueue.front().isGoodFrame()) {
            MACFrame macFrame(m_recvQueue.front());
            if (macFrame.isGood()) {
                uint8_t id = macFrame.getId();
                switch (macFrame.getType()) {
                    case MACType::DATA:
#ifdef VERBOSE_MAC
                        std::cout << "MACReceiver: FrameDetector: DATA detected, id: " << (int) macFrame.getId()
                                  << " length: " << macFrame.getLength() << "\n";
#endif
                        if (m_reorderQueue.addPacket(std::move(macFrame)))
                            MACManager::get().macTransmitter->SendACK(id);
                        while (m_reorderQueue.isReady())
                            m_packetQueue.push_back(m_reorderQueue.getPacketOrdered());
                        break;
                    case MACType::ACK:
#ifdef VERBOSE_MAC
                        std::cout << "MACReceiver: FrameDetector: ACK detected, id: " << (int) macFrame.getId() << "\n";
#endif
                        MACManager::get().macTransmitter->ReplyReceived(macFrame);
                        break;
                }
            }
        }
        m_recvQueue.pop_front();
    }
}

void MACLayerReceiver::frameReceived(Frame &&frame) {
    m_recvQueue.push_back(std::move(frame));
}

int MACLayerReceiver::RecvPacket(uint8_t *out) {
    uint16_t len = m_packetQueue.front().serialize(out, false);
    m_packetQueue.pop_front();
    return len;
}

MACLayerTransmitter::MACLayerTransmitter(std::shared_ptr<AudioDevice> audioDevice)
        : m_slidingWindow(Config::SLIDING_WINDOW_SIZE),
          m_asyncSender(std::move(audioDevice)) {
    MACThread = std::make_unique<std::thread>([this]() { MACThreadTransStart(); });
}

MACLayerTransmitter::~MACLayerTransmitter() {
    running = false;
    if (MACThread) {
        m_sendQueue.push_back(MACFrame());
        MACThread->join();
    }
}

void MACLayerTransmitter::ReplyReceived(MACFrame &reply) {
    jassert(reply.getType() == MACType::ACK);

    if (m_slidingWindow.removePacket(reply.getId())) {
        m_cvAck.notify_one();
        return;
    }

    std::cout << "SENDER: Received bad reply "
              << (int) reply.getHeader().id << " type "
              << (int) reply.getHeader().type << ".\n";
}

void MACLayerTransmitter::MACThreadTransStart() {
    std::future<void> asyncFutures[Config::SLIDING_WINDOW_SIZE];
    auto t1 = std::chrono::system_clock::now();

    while (running) {
        int windowIdx = m_slidingWindow.addPacket(m_sendQueue.front());

        if (windowIdx != -1) {
            m_sendQueue.pop_front();
            asyncFutures[windowIdx] = std::async(std::launch::async,
                                                 [this, windowIdx]() { return SlidingWindowSender(windowIdx); });
        }

        std::unique_lock<std::mutex> lock_ack(m_mAck);
        m_cvAck.wait_for(lock_ack, Config::ACK_TIMEOUT / Config::SLIDING_WINDOW_SIZE,
                         [this, windowIdx]() { return windowIdx != -1 || !running; });
    }

    auto t2 = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "\n";
    for (auto &asyncFuture: asyncFutures) {
        asyncFuture.get();
    }
}

void MACLayerTransmitter::SendPacket(const uint8_t *data, int len) {
    jassert(len <= Config::MACDATA_PER_FRAME);
    m_sendQueue.push_back(MACFrame(data, static_cast<uint16_t>(len)));
}

void MACLayerTransmitter::SendACK(uint8_t id) {
    m_asyncSender.SendACKAsync(id);
}

void MACLayerTransmitter::SlidingWindowSender(int windowIdx) {
    uint8_t macId;
    int resendCount = 0;
    auto sendTime = std::chrono::system_clock::now();

    while (m_slidingWindow.getStatus(windowIdx)) {
        MACFrame macFrame(m_slidingWindow.getPacket(windowIdx));
        macId = macFrame.getId();
        /* There may be one situation that the status is changed after
         * getStatus before send. */
        m_asyncSender.SendMACFrameAsync(macFrame);
        if (m_slidingWindow.waitReply(windowIdx))
            break;

        resendCount += 1;
        if (resendCount >= 20) {
            std::cout << "Link Error: no ACK received after 20 retries.\n";
            running = false;
            return;
        }
    }

    auto recvTime = std::chrono::system_clock::now();
#ifdef VERBOSE_MAC
    std::cout << "MACTransmitter: Time to ACK " << (int) macId << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(recvTime - sendTime).count() << "\n";
#endif
}


CSMASenderQueue::CSMASenderQueue(std::shared_ptr<AudioDevice> audioDevice)
        : m_audioDevice(std::move(audioDevice)),
          running(true) {
    m_senderThread = std::make_unique<std::thread>([this]() { SenderStart(); });
}

CSMASenderQueue::~CSMASenderQueue() {
    running = false;
    if (m_senderThread) {
        m_senderThread->detach();
    }
}

void CSMASenderQueue::SenderStart() {
    while (running) {
        int retryNum = 0;
        /* wait until the queue is not empty */
        m_queue.front();
        while (!m_queue.empty() && running) {
            retryNum++;
            if (m_audioDevice->getChannelState() == AudioDevice::CN_BUSY)
                std::this_thread::sleep_for(5ms);

            if (m_audioDevice->getChannelState() == AudioDevice::CN_IDLE) {
                m_audioDevice->sendFrame(m_queue.front().second);
                m_hasFrame.erase(m_queue.front().first);
                m_queue.pop_front();
                break;
            }

            std::cout << "Channel busy, retrying " << retryNum << "\n";
            if (retryNum >= 10)
                retryNum = 10;
            std::this_thread::sleep_for(Config::BACKOFF_TSLOT * (int) pow(2, retryNum - 1));
        }
    }
}

void CSMASenderQueue::SendMACFrameAsync(const MACFrame &macFrame) {
    uint8_t buffer[Config::DATA_PER_FRAME / 8];
    uint16_t key = macFrame.getHeader().crc16;

    if (m_hasFrame.count(key))
        return;
#ifdef VERBOSE_MAC
    std::cout << "MACTransmitter: Send Frame: " << (int) macFrame.getId() << "\n";
#endif
    macFrame.serialize(buffer, true);
    m_queue.push_back(std::make_pair(key, AudioFrame(Frame(buffer, macFrame.getLength() + sizeof(MACHeader)))));
}

void CSMASenderQueue::SendACKAsync(uint8_t id) {
    uint8_t buffer[Config::DATA_PER_FRAME / 8];
    MACFrame ackFrame(id, MACType::ACK);
    uint16_t key = ackFrame.getHeader().crc16;

    if (m_hasFrame.count(key))
        return;
#ifdef VERBOSE_MAC
    std::cout << "MACTransmitter: Send ACK: " << (int) ackFrame.getId() << "\n";
#endif
    ackFrame.serialize(buffer, true);
    m_queue.push_back(std::make_pair(key, AudioFrame(Frame(buffer, sizeof(MACHeader)))));
}
