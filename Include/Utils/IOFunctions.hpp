#pragma once

#ifndef __IOFUNCTIONS_HPP__
#define __IOFUNCTIONS_HPP__

#include "Physical/Audio.h"

/* Here `inline` hints the linker to choose arbitary one definition. */
static void __bitToByte(const DataType &bitArray, DataType &byteArray) {
    for (int i = 0; i < bitArray.size(); i += 8) {
        uint8_t val = 0;
        for (int j = 0; j < 8; j++)
            val |= bitArray[i + j] << ((i + j) % 8);
        byteArray.set(i / 8, val);
    }
}

inline DataType bitToByte(const DataType &bitArray) {
    DataType byteArray;
    byteArray.resize(bitArray.size() / 8);
    __bitToByte(bitArray, byteArray);
    return std::move(byteArray);
}

static void __byteToBit(DataType &bitArray, const DataType &byteArray) {
    bitArray.resize(byteArray.size() * 8);
    for (int i = 0; i < bitArray.size(); i++) {
        bitArray.set(i, ((1 << (i % 8)) & (byteArray[i / 8])) >> (i % 8));
    }
}

inline DataType byteToBit(const DataType &byteArray) {
    DataType bitArray;
    __byteToBit(bitArray, byteArray);
    return std::move(bitArray);
}

#endif