#pragma once

#ifndef __ACK_H__
#define __ACK_H__

#include "Physical/Frame.h"
#include "Config.h"

class ACK : public Frame
{
public:
    ACK(const uint8_t *pdata);
    ~ACK() = default;
};

#endif