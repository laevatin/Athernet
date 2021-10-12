#ifndef __RINGBUFFER_HPP__
#define __RINGBUFFER_HPP__

#include <type_traits>
#include <cstring>
#include <iostream>

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
    RingBuffer() {};
    ~RingBuffer() {};

    void write(const Type* data, std::size_t len)
    {
        if (fillCount + len > CAPACITY)
            std::cerr << "Ring Buffer Overflow\n";

        if ((uint16_t)(head + len) < head)
        {
            memcpy(buffer + head, data, sizeof(Type) * (UINT16_MAX - head));
            memcpy(buffer, data + (UINT16_MAX - head), sizeof(Type) * (uint16_t)(head + len));
        }
        else
            memcpy(buffer + head, data, sizeof(Type) * len);

        head += len;
        fillCount += len;
    }

    void read(Type* data, std::size_t len)
    {
        if (fillCount < len)
            std::cerr << "Ring Buffer Underflow\n";

        if ((uint16_t)(tail + len) < tail)
        {
            memcpy(data, buffer + tail, sizeof(Type) * (UINT16_MAX - tail));
            memcpy(data + (UINT16_MAX - tail), buffer, sizeof(Type) * (uint16_t)(tail + len));
        }
        else
            memcpy(data, buffer + tail, sizeof(Type) * len);

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

private:
    Type buffer[CAPACITY];
    uint16_t head = 0;
    uint16_t tail = 0;
    std::size_t fillCount = 0;
};


#endif  