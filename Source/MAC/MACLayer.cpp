#include "MAC/MACLayer.h"
#include "Physical/Audio.h"
#include "MAC/MACManager.h"

#include <chrono>
#include <memory>
#include <utility>

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
                    case MACType::PING_REQ:
#ifdef VERBOSE_MAC
                        std::cout << "RECIVER: FrameDetector: MACPING_REQ detected.\n";
#endif
                        MACManager::get().macTransmitter->SendPing(MACType::PING_REP);
                        break;
                    case MACType::PING_REP:
                        [[fallthrough]];
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
        : m_txState(MACLayerTransmitter::IDLE) {
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
    switch (reply.getType()) {
        case MACType::PING_REP:
            if (m_txState == SEND_PING) {
                m_txState = PING_RECEIVED;
                m_cvAck.notify_one();
                return;
            }
            break;

        case MACType::ACK:
            if (m_txState != SEND_PING && m_sentWindow.count(reply.getId())) {
                m_sentWindow.erase(reply.getId());
                m_lastId = reply.getId();
                m_txState = ACK_RECEIVED;
                m_cvAck.notify_one();
                return;
            }
            break;

        default:
            break;
    }

    std::cout << "SENDER: Received bad reply "
              << (int) reply.getHeader().id << " type "
              << (int) reply.getHeader().type << ".\n";
}

void MACLayerTransmitter::MACThreadTransStart() {
    int resendCount = 0;
    while (true) {
        std::unique_lock<std::mutex> lock_queue(m_mSend);
        m_cvSend.wait(lock_queue, [this]() {
            return !m_sendQueue.empty() || !m_sentWindow.empty() || !running;
        });

        if (!running)
            break;

        if (!m_sendQueue.empty()) {
            m_sentWindow.insert(std::make_pair(m_sendQueue.front().getId(),
                                               std::move(m_sendQueue.front())));
            m_sendQueue.pop_front();
        }

        auto now = std::chrono::system_clock::now();
        m_asyncSender->SendMACFrameAsync(m_sentWindow.begin()->second);
        lock_queue.unlock();

        std::unique_lock<std::mutex> lock_ack(m_mAck);
        if (m_cvAck.wait_until(lock_ack, now + Config::ACK_TIMEOUT, [this]() {
            return m_txState == PING_RECEIVED || m_txState == ACK_RECEIVED;
        })) {
            if (m_txState == PING_RECEIVED) {
                auto recv = std::chrono::system_clock::now();
                std::cout << "MACLayer: Get Ping After: "
                          << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count()
                          << "ms\n";
            } else if (m_txState == ACK_RECEIVED) {
                resendCount = 0;
                auto recv = std::chrono::system_clock::now();
#ifdef VERBOSE_MAC
                std::cout << "SENDER: Time to ACK " << (int) m_lastId << ": "
                          << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() << "\n";
#endif
                m_sentWindow.erase(m_lastId);
            }
        } else {
            resendCount += 1;
            if (resendCount >= 100) {
                std::cout << "Link Error: no ACK received after 10 retries.\n";
                break;
            }
        }
    }
}

void MACLayerTransmitter::SendPacket(const uint8_t *data, int len) {
    jassert(len <= Config::MACDATA_PER_FRAME);

    std::lock_guard<std::mutex> guard(m_mSend);

    m_txState = SEND_DATA;
    m_sendQueue.emplace_back(data, static_cast<uint16_t>(len));
    m_cvSend.notify_one();
}

void MACLayerTransmitter::SendPing(MACType pingType) {
    if (pingType != MACType::PING_REQ && pingType != MACType::PING_REP)
        return;

    std::lock_guard<std::mutex> guard(m_mSend);
    m_txState = SEND_PING;
    m_sendQueue.emplace_back(Config::MACPING_ID, pingType);
    m_cvSend.notify_one();
}

void MACLayerTransmitter::SendACK(uint8_t id) {
    m_asyncSender->SendACKAsync(id);
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
                Sleep(10);

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
    std::cout << "SENDER: Send Frame: " << (int) macFrame.getId() << "\n";
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
    std::cout << "SENDER: Send ACK: " << (int) ackFrame.getId() << "\n";
#endif
    ackFrame.serialize(buffer, true);
    m_queue.emplace_back(key, AudioFrame(Frame(buffer, sizeof(MACHeader))));

    m_cvQueue.notify_one();
}
