#pragma once

#ifndef __ANET_H__
#define __ANET_H__

#include <JuceHeader.h>
#include <string>
#include <memory>
#include "UDP/UDP.h"
#include "NAT/Ping.h"
#include "Physical/Audio.h"
#include "Config.h"

enum ANetIPTag
{
    DATA, 
    PING
};

struct ANetIP
{
    uint32_t ip_src;
    uint32_t ip_dst;
    enum ANetIPTag tag;
};

struct ANetUDP
{
    uint16_t udp_src_port;
    uint16_t udp_dst_port;
    uint16_t udp_len;
};

struct ANetPing
{
    ANetPing(const char *destIP);
    char destIP[24];
    // int RTT;
    bool success;
};

struct ANetPacket 
{
    ANetPacket(uint32_t srcip, uint32_t dstip, uint16_t srcport, uint16_t dstport, const uint8_t* data, uint16_t len);
    ANetPacket() = default;
    struct ANetIP ip;
    struct ANetUDP udp;
    uint8_t payload[Config::PACKET_PAYLOAD];
};

class ANet
{
public:
    ANet(const char* self_ip, const char* self_port, const char* dest_ip, const char* dest_port, int node);
    ANet(ANet &other) = delete;
    ~ANet();
    void SendData(const uint8_t* data, int len, int dest_node);
    int RecvData(uint8_t* out, int outlen, int from_node);
    void Gateway(int from, int to);
    void SendPing(const char *pingIP);

private:
    std::unique_ptr<UDPClient> m_udpClient;
    std::unique_ptr<UDPServer> m_udpServer;
    std::unique_ptr<AudioIO> m_audioIO;
    // std::unique_ptr<IcmpPing> m_icmpPing;
    
    uint32_t m_selfIP;
    uint32_t m_destIP;
    uint16_t m_selfPort;
    uint16_t m_destPort;
    int m_node;
};

#endif