#pragma once

#ifndef __ANET_H__
#define __ANET_H__

#include <JuceHeader.h>
#include "Config.h"

struct ANetIP
{
    uint32_t ip_src;
    uint32_t ip_dst;
};

struct ANetUDP
{
    uint16_t udp_src;
    uint16_t udp_dst;
    uint16_t udp_len;
};

struct ANetPacket 
{
    struct ANetIP ip;
    struct ANetUDP udp;
    uint8_t payload[Config::PACKET_PAYLOAD];
};

class ANet
{
    ANet(const char *self_ip, const char *dest_ip);
};

#endif