#pragma once

#ifndef __ANET_H__
#define __ANET_H__

#include <JuceHeader.h>
#include <string>
#include <memory>
#include <map>
#include "UDP/UDP.h"
#include "ANet/Ping.h"
#include "Physical/Audio.h"
#include "Config.h"

enum ANetIPTag;

struct ANetIP {
    uint32_t ip_src;
    uint32_t ip_dst;
    enum ANetIPTag tag;
};

struct ANetUDP {
    uint16_t udp_src_port;
    uint16_t udp_dst_port;
    uint16_t udp_len;
};

struct ANetPing {
    uint8_t success;
};

struct ANetPacket {
    ANetPacket(uint32_t srcip, uint32_t dstip, uint16_t srcport, uint16_t dstport, const uint8_t *data, uint16_t len);

    ANetPacket() = default;

    struct ANetIP ip{};
    struct ANetUDP udp{};
    uint8_t payload[Config::IP_PACKET_PAYLOAD]{};
};

class ANetClient {
public:
    ANetClient(const char *dest_ip, const char *dest_port, bool isAthernet);

    ANetClient(ANetClient &other) = delete;

    void SendData(const uint8_t *data, int len);

    void SendString(const std::string &str);

    void SendInt(int i);

    void SendPing(const char *target, const char *src = nullptr);

    void StartPingReq();

    void StopPingReq();

    void StartRecvReq();

    void SendPing();

private:

    void SendPacketWithType(ANetIPTag type);
    std::unique_ptr<UDPClient> m_udpClient;
    std::unique_ptr<AudioIO> m_audioIO;
    std::unique_ptr<IcmpPing> m_icmpPing;

    uint32_t m_selfIP{};
    uint16_t m_selfPort;
    uint32_t m_destIP{};
    uint16_t m_destPort;
    bool m_isAthernet;
};

class ANetServer {
public:
    ANetServer(const char *open_port, bool isAthernet);

    ANetServer(ANetServer &other) = delete;

    void RecvString(std::string &str);

    int RecvInt();

    int RecvData(uint8_t *out, int outlen);

    void ReplyPing();

private:
    std::unique_ptr<UDPServer> m_udpServer;
    std::unique_ptr<AudioIO> m_audioIO;

    uint16_t m_openPort;
    bool m_isAthernet;
};

class ANetGateway {
public:
    ANetGateway(const char *anet_port, const char *out_port);

    void StartForwarding();

private:

    void ATNToInternet();

    void InternetToATN();

    static void OnPingArrive(const char *src_ip);
    static int replyCount;

    uint16_t m_anetPort;
    uint16_t m_outPort;
    AudioIO m_audioIO;
    UDPServer m_udpServer;
    std::map<uint32_t, std::unique_ptr<ANetClient>> m_NATTable;
};

#endif