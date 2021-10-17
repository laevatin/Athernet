#pragma once

#ifndef __MODULATOR_H__
#define __MODULATOR_H__

#include "Frame.h"

class Modulator {

    static void modulate(const DataType &data, int start, DataType &out);

    static void demodulate(const float *samples, DataType &out);

};


#endif