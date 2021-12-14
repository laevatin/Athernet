#pragma once

#ifndef __ANET_H__
#define __ANET_H__

#include <JuceHeader.h>
#include <string>
#include <memory>
#include "UDP/UDP.h"
#include "ANet/Ping.h"
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

class ANetClient
{
public:
    ANetClient(const char *dest_ip, const char *dest_port, bool isAthernet);
    ANetClient(ANetClient &other) = delete;

    void SendData(const uint8_t* data, int len, int dest_node);

    void SendPing(const char *pingIP);

private:
    std::unique_ptr<UDPClient> m_udpClient;
    std::unique_ptr<AudioIO> m_audioIO;
    
    uint32_t m_selfIP;
    uint16_t m_selfPort;
    uint32_t m_destIP;
    uint16_t m_destPort;
    bool m_isAthernet;
};

class ANetServer
{
public:
    ANetServer(const char *open_port, bool isAthernet);
    ANetServer(ANetServer &other) = delete;
    int RecvData(uint8_t* out, int outlen);

private:
    std::unique_ptr<UDPServer> m_udpServer;
    std::unique_ptr<AudioIO> m_audioIO;
    
    uint16_t m_openPort;
    bool m_isAthernet;
};

#endif