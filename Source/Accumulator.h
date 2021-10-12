#pragma once

#ifndef __ACCUMULATOR_H__
#define __ACCUMULATOR_H__

#include <vector>

/**
 * This class is a template class for the circular accumulator
 * which is used to keep a running sum of the past #size samples
 */

template<typename T>
struct Accumulator
{
    T sum;
    std::vector<T> samples;
    int index;
    int size;

    Accumulator(int _size)
    {
        size = _size;
        samples.resize(size);
        index = 0;
        for (int x = 0; x < size; x++) 
            samples[x] = T(0);
        sum = T(0);
    }

    void add(T sample)
    {
        sum -= samples[index];
        sum += sample;
        samples[index++] = sample;
        if (index >= size) 
            index = 0;
    }

};

#endif
