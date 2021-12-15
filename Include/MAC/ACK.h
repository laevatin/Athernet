#pragma once

#ifndef __ACK_H__
#define __ACK_H__

#include "Physical/Frame.h"
class MACHeader;

class ACK : public Frame
{
public:
    /* Create an ack to send */
    ACK(const uint8_t *pdata, uint8_t id);

    /* Receive an ack */
    ACK(MACHeader *header);
    ACK(ACK&& other);

    ACK(const ACK& other) = default;

    ~ACK() = default;
};

#endif