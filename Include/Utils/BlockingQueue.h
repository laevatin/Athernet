#pragma once

#ifndef __BLOCKING_QUEUE_H__
#define __BLOCKING_QUEUE_H__

#include <list>
#include <condition_variable>

template<typename T>
class BlockingQueue : private std::list<T> {
public:
    BlockingQueue() = default;
    ~BlockingQueue() = default;

    void push_back(T&& val) {
        std::lock_guard guard(m_mQueue);
        std::list<T>::push_back(std::forward<T>(val));
        m_cvQueue.notify_one();
    };

    void pop_front() {
        std::lock_guard guard(m_mQueue);
        std::list<T>::pop_front();
    };

    T& front() {
        std::unique_lock<std::mutex> lock(m_mQueue);
        m_cvQueue.wait(lock, [this]() { return !std::list<T>::empty(); });
        return std::list<T>::front();
    };

    bool empty() {
        std::lock_guard guard(m_mQueue);
        return std::list<T>::empty();
    }

private:
    std::mutex m_mQueue;
    std::condition_variable m_cvQueue;
};

#endif