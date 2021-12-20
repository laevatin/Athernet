#include "MAC/MACLayer.h"
#include "Physical/Audio.h"
#include "MAC/MACManager.h"

#include <chrono>
#include <memory>
#include <utility>
#include <future>
#include <windows.h>

using namespace std::chrono_literals;

MACLayer::MACLayer()
        : running(true) {}

MACLayerReceiver::MACLayerReceiver() {
    MACThread = std::make_unique<std::thread>([this]() { MACThreadRecvStart(); });
}

MACLayerReceiver::~MACLayerReceiver() {
    running = false;
    if (MACThread) {
        m_cvQueue.notify_one();
        MACThread->join();
    }
}

void MACLayerReceiver::MACThreadRecvStart() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_mQueue);

        m_cvQueue.wait(lock, [this]() {
            return !m_recvQueue.empty() || !running;
        });

        if (!running)
            break;

        if (m_recvQueue.front().isGoodFrame()) {
            MACFrame macFrame(m_recvQueue.front());
            if (macFrame.isGood()) {
                switch (macFrame.getType()) {
                    case MACType::DATA:
#ifdef VERBOSE_MAC
                        std::cout << "RECIVER: FrameDetector: DATA detected, id: " << (int) macFrame.getId()
                                  << " length: " << macFrame.getLength() << "\n";
#endif
                        MACManager::get().macTransmitter->SendACK(macFrame.getId());
                        m_mPacketQueue.lock();
                        m_packetQueue.push_back(std::move(macFrame));
                        m_mPacketQueue.unlock();
                        m_cvPacketQueue.notify_one();
                        break;
                    case MACType::ACK:
#ifdef VERBOSE_MAC
                        std::cout << "RECIVER: FrameDetector: ACK detected, id: " << (int) macFrame.getId() << "\n";
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
    std::lock_guard<std::mutex> guard(m_mQueue);
    m_recvQueue.push_back(std::move(frame));
    m_cvQueue.notify_one();
}

int MACLayerReceiver::RecvPacket(uint8_t *out) {
    std::unique_lock<std::mutex> lock_packet(m_mPacketQueue);
    m_cvPacketQueue.wait(lock_packet, [this]() { return !m_packetQueue.empty(); });

    uint16_t len = m_packetQueue.front().serialize(out, false);
    m_packetQueue.pop_front();
    return len;
}

MACLayerTransmitter::MACLayerTransmitter(std::shared_ptr<AudioDevice> audioDevice)
        : m_slidingWindow(Config::SLIDING_WINDOW_SIZE) {
    m_asyncSender = std::make_unique<CSMASenderQueue>(audioDevice);
    MACThread = std::make_unique<std::thread>([this]() { MACThreadTransStart(); });
}

MACLayerTransmitter::~MACLayerTransmitter() {
    running = false;
    if (MACThread) {
        m_cvSend.notify_one();
        MACThread->join();
    }
}

void MACLayerTransmitter::ReplyReceived(MACFrame &reply) {
    std::lock_guard<std::mutex> guard(m_mSend);
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
    int resendCount = 0;
    std::future<void> asyncFutures[Config::SLIDING_WINDOW_SIZE];
    while (true) {
        std::unique_lock<std::mutex> lock_queue(m_mSend);
        m_cvSend.wait(lock_queue, [this]() {
            return !m_sendQueue.empty() || !running;
        });

        if (!running)
            break;

        int windowIdx = m_slidingWindow.addPacket(m_sendQueue.front());

        if (windowIdx != -1) {
            m_sendQueue.pop_front();
            asyncFutures[windowIdx] = std::async(std::launch::async,
                                                 [this, windowIdx]() { return SlidingWindowSender(windowIdx); });
        }

        lock_queue.unlock();
        std::unique_lock<std::mutex> lock_ack(m_mAck);
        m_cvAck.wait_for(lock_ack, Config::ACK_TIMEOUT);
    }

    for (auto & asyncFuture : asyncFutures) {
        asyncFuture.get();
    }
}

void MACLayerTransmitter::SendPacket(const uint8_t *data, int len) {
    jassert(len <= Config::MACDATA_PER_FRAME);

    std::lock_guard<std::mutex> guard(m_mSend);
    m_sendQueue.emplace_back(data, static_cast<uint16_t>(len));
    m_cvSend.notify_one();
}

void MACLayerTransmitter::SendACK(uint8_t id) {
    m_asyncSender->SendACKAsync(id);
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
        m_asyncSender->SendMACFrameAsync(macFrame);
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
        m_cvQueue.notify_one();
        m_senderThread->join();
    }
}

void CSMASenderQueue::SenderStart() {
    while (true) {
        std::unique_lock<std::mutex> lock_queue(m_mQueue);
        m_cvQueue.wait(lock_queue, [this]() {
            return !m_queue.empty() || !running;
        });

        if (!running)
            return;

        int retryNum = 0;
        while (!m_queue.empty() && running) {
            retryNum++;
            if (m_audioDevice->getChannelState() == AudioDevice::CN_BUSY)
                std::this_thread::sleep_for(10ms);

            if (m_audioDevice->getChannelState() == AudioDevice::CN_IDLE) {
                m_audioDevice->sendFrame(m_queue.front().second);
                m_hasFrame.erase(m_queue.front().first);
                m_queue.pop_front();
                break;
            }

            lock_queue.unlock();
            std::cout << "Channel busy, retrying " << retryNum << "\n";
            if (retryNum >= 10)
                retryNum = 10;
            Sleep(Config::BACKOFF_TSLOT * (int) pow(2, retryNum - 1));
            lock_queue.lock();
        }
    }
}

void CSMASenderQueue::SendMACFrameAsync(const MACFrame &macFrame) {
    uint8_t buffer[Config::DATA_PER_FRAME / 8];
    uint16_t key = macFrame.getHeader().crc16;

    std::lock_guard<std::mutex> guard(m_mQueue);
    if (m_hasFrame.count(key))
        return;
#ifdef VERBOSE_MAC
    std::cout << "MACTransmitter: Send Frame: " << (int) macFrame.getId() << "\n";
#endif
    macFrame.serialize(buffer, true);
    m_queue.emplace_back(key, AudioFrame(Frame(buffer, macFrame.getLength() + sizeof(MACHeader))));
    m_cvQueue.notify_one();
}

void CSMASenderQueue::SendACKAsync(uint8_t id) {
    uint8_t buffer[Config::DATA_PER_FRAME / 8];
    MACFrame ackFrame(id, MACType::ACK);
    uint16_t key = ackFrame.getHeader().crc16;

    std::lock_guard<std::mutex> guard(m_mQueue);
    if (m_hasFrame.count(key))
        return;
#ifdef VERBOSE_MAC
    std::cout << "MACTransmitter: Send ACK: " << (int) ackFrame.getId() << "\n";
#endif
    ackFrame.serialize(buffer, true);
    m_queue.emplace_back(key, AudioFrame(Frame(buffer, sizeof(MACHeader))));

    m_cvQueue.notify_one();
}
