#ifndef __RINGBUFFER_HPP__
#define __RINGBUFFER_HPP__

#include <type_traits>
#include <cstring>
#include <iostream>
#include <functional>

#define CAPACITY 65536

/**
 * This ring buffer is fixed size of 65536 elements
 * since 65536 is the size of uint16_t
 */

template <typename Type>
class RingBuffer
{
    static_assert(std::is_floating_point<Type>::value, "FP type required.");

public:
    RingBuffer() = default;
    ~RingBuffer() = default;

    void write(const Type* data, std::size_t len)
    {
#ifdef DEBUG
        if (fillCount + len > CAPACITY)
            std::cerr << "Ring Buffer Overflow\n";
#endif
        if ((uint16_t)(head + len) < head)
        {
            memcpy(buffer + head, data, sizeof(Type) * (UINT16_MAX + 1 - head));
            memcpy(buffer, data + (UINT16_MAX + 1 - head), sizeof(Type) * (uint16_t)(head + len));
        }
        else
            memcpy(buffer + head, data, sizeof(Type) * len);

        head += len;
        fillCount += len;
    }

    void read(Type* data, std::size_t len)
    {
#ifdef DEBUG
        if (fillCount < len)
            std::cerr << "Ring Buffer Underflow\n";
#endif
        if ((uint16_t)(tail + len) < tail)
        {
            memcpy(data, buffer + tail, sizeof(Type) * (UINT16_MAX + 1 - tail));
            memcpy(data + (UINT16_MAX + 1 - tail), buffer, sizeof(Type) * (uint16_t)(tail + len));
        }
        else
            memcpy(data, buffer + tail, sizeof(Type) * len);

        tail += len;
        fillCount -= len;
    }

    template <typename T>
    T peek(std::function<T(int, const Type *, const Type *)> func, const Type *data, std::size_t len, int offset)
    {
#ifdef DEBUG
        if (fillCount < len + offset)
            std::cerr << "Peek out of bound\n";
#endif
        uint16_t newTail = tail + offset;
        T ret = T(0);

        if ((uint16_t)(newTail + len) < newTail)
        {
            ret += func((int)UINT16_MAX + 1 - (int)newTail, data, buffer + newTail);
            ret += func((int)((uint16_t)(newTail + len)), data + (UINT16_MAX + 1 - newTail), buffer);
        }
        else
            ret += func((int)len, data, buffer + newTail);

        return ret;
    }

    void discard(std::size_t len)
    {
#ifdef DEBUG
        if (fillCount < len)
            std::cerr << "discard: Ring Buffer Underflow\n";
#endif
        tail += len;
        fillCount -= len;
    }

    void reset()
    {
        head = 0;
        tail = 0;
        fillCount = 0;
    }

    bool hasEnoughSpace(std::size_t len) const
    {
        return fillCount + len <= CAPACITY;
    }

    bool hasEnoughElem(std::size_t elem) const
    {
        return fillCount >= elem;
    }

    std::size_t size() const
    {
        return fillCount;
    }

    std::size_t avail() const
    {
        return CAPACITY - fillCount;
    }

private:
    Type buffer[CAPACITY];
    uint16_t head = 0;
    uint16_t tail = 0;
    std::size_t fillCount = 0;
};

#endif  