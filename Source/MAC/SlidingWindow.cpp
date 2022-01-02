#include "MAC/SlidingWindow.h"

SlidingWindow::SlidingWindow(size_t size)
        : m_windowData(size),
          m_windowCV(size),
          m_left(0),
          m_right(-1) {}

bool SlidingWindow::removePacket(uint8_t id) {
    std::lock_guard<std::mutex> guard(m_mWindow);
    for (int i = 0; i < m_windowData.size(); i++) {
        if (m_windowData[i].second.getId() == id) {
            if (m_windowData[i].first) {
                m_windowData[i].first = false;
                updateLeft(id);
                m_windowCV[i].notify_one();
#ifdef SLIDING_WINDOW_DEBUG
                std::cout << (int)id << " received, notifying sender " << i << "\n";
#endif
                return true;
            }
            return false;
        }
    }
    return false;
}

int SlidingWindow::addPacket(MACFrame &macFrame) {
    uint8_t currWindowSize = m_right - m_left + 1;
#ifdef SLIDING_WINDOW_DEBUG
    std::cout << "add packet " << (int)macFrame.getId() << " window size " << (int)currWindowSize << "\n";
#endif
    if (currWindowSize >= m_windowData.size() || !macFrame.isGood())
        return -1;

    jassert(macFrame.getId() == (uint8_t) (m_right + 1));
    std::lock_guard<std::mutex> guard(m_mWindow);
    for (int i = 0; i < m_windowData.size(); i++) {
        if (!m_windowData[i].first) {
            m_windowData[i].first = true;
            m_windowData[i].second = macFrame;
            m_right += 1;
            return i;
        }
    }
    return -1;
}

bool SlidingWindow::waitReply(int idx) {
    std::unique_lock<std::mutex> lock(m_mWindow);
    return m_windowCV[idx].wait_for(lock,
                                    Config::ACK_TIMEOUT,
                                    [this, idx]() { return !m_windowData[idx].first; });
}

/* For thread safety considerations, the return value should be copied */
MACFrame SlidingWindow::getPacket(int idx) {
    jassert(idx >= 0 && idx < m_windowData.size());
    std::lock_guard<std::mutex> guard(m_mWindow);
    return m_windowData[idx].second;
}

bool SlidingWindow::getStatus(int idx) {
    jassert(idx >= 0 && idx < m_windowData.size());
    std::lock_guard<std::mutex> guard(m_mWindow);
    return m_windowData[idx].first;
}

void SlidingWindow::updateLeft(uint8_t lastRecv) {
    if (lastRecv == m_left) {
        uint8_t diff = m_right - m_left + 1;
        for (auto &&p: m_windowData)
            if (p.first)
                diff = std::min(diff, static_cast<uint8_t>(p.second.getId() - m_left));

        m_left += diff;
    }
}

ReorderQueue::ReorderQueue(size_t size)
        : m_windowData(size),
          m_left(0),
          m_right(size - 1),
          m_isReady(false) {}

bool ReorderQueue::addPacket(MACFrame &&macFrame) {
    uint8_t cur = macFrame.getId() - m_left;
    uint8_t maximum = m_right - m_left;
    if (!macFrame.isGood() || cur > maximum) {
#ifdef VERBOSE_MAC
        std::cout << "MACReceiver: received bad frame with id " << (int) macFrame.getId() << "\n";
#endif
        return false;
    }

    for (auto &&p: m_windowData) {
        if (!p.first) {
            p.first = true;
            p.second = std::move(macFrame);
            break;
        }
    }

    updateReady();
    return true;
}

bool ReorderQueue::isReady() const {
    return m_isReady;
}

MACFrame ReorderQueue::getPacketOrdered() {
    jassert(m_isReady);

    for (auto &&p: m_windowData)
        if (p.first && p.second.getId() == m_left) {
            p.first = false;
            nextWindow();
            updateReady();
            return std::move(p.second);
        }

    return {};
}

void ReorderQueue::updateReady() {
    m_isReady = false;
    for (auto &&p: m_windowData)
        if (p.first && p.second.getId() == m_left)
            m_isReady = true;
}

void ReorderQueue::nextWindow() {
    m_left += 1;
    m_right += 1;
}

