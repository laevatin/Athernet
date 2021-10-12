#pragma once

#ifndef __DEMODULATOR_H__
#define __DEMODULATOR_H__

#include "Audio.h"
#include "Frame.h"

class Demodulator 
{
public:
    Demodulator();
    ~Demodulator();

    

private:

    friend class AudioDevice;
};

#endif