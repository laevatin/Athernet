#pragma once

#ifndef __SLIDING_WINDOW_H__
#define __SLIDING_WINDOW_H__

#include <vector>
#include <map>
#include <mutex>
#include <MAC/MACFrame.h>

class SlidingWindow {

public:
    /* window size should be less than 127 */
    explicit SlidingWindow(size_t size);
    ~SlidingWindow() = default;

    bool removePacket(uint8_t id);
    /* this function assumes the coming IDs are in order */
    int addPacket(MACFrame &macFrame);

    bool waitReply(int idx);

    bool getStatus(int idx);
    MACFrame getPacket(int idx);

private:
    void updateLeft(uint8_t lastRecv);
    /**
     * Since the window size would not be very large, the
     * O(n) search is acceptable.
     */
    uint8_t m_left;
    uint8_t m_right;
    std::vector<std::pair<std::atomic<bool>, MACFrame>> m_windowData;
    std::vector<std::condition_variable> m_windowCV;
    std::mutex m_mWindow;
};

class ReorderQueue {

public:
    explicit ReorderQueue(size_t size);
    ~ReorderQueue() = default;

    bool addPacket(MACFrame &&macFrame);

    [[nodiscard]] bool isReady() const;

    MACFrame getPacketOrdered();

private:

    void updateReady();

    void nextWindow();

    uint8_t m_left;
    uint8_t m_right;
    bool m_isReady;
    std::vector<std::pair<std::atomic<bool>, MACFrame>> m_windowData;

};

#endif