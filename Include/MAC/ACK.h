#pragma once

#ifndef __ACK_H__
#define __ACK_H__

#include "Physical/Frame.h"
#include "Config.h"
#include "MAC/MACFrame.h"

class ACK : public Frame
{
public:
    /* Create an ack to send */
    ACK(const uint8_t *pdata);

    /* Receive an ack */
    ACK(MACHeader *header);
    ~ACK() = default;
};

#endif