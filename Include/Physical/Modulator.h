#pragma once

#ifndef __MODULATOR_H__
#define __MODULATOR_H__

#include "Frame.h"
#include "AudioFrame.h"
#include <JuceHeader.h>

class Modulator {

public:
    static void modulate(AudioFrame& audioFrame, const DataType& bitArray);

    static void demodulate(const float *samples, DataType &out);

};


#endif