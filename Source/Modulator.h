#pragma once

#ifndef __MODULATOR_H__
#define __MODULATOR_H__

#include "Frame.h"
#include <JuceHeader.h>

class Modulator {

public:
    static void modulate(const DataType& data, int start, Frame& frame);

    static void demodulate(const float *samples, DataType &out);

};


#endif