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
                m_windowCV[i].second.notify_one();
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
    jassert(macFrame.getId() == (uint8_t)(m_right + 1));
    if (currWindowSize >= m_windowData.size())
        return -1;

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
    return m_windowCV[idx].second.wait_for(lock,
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

