#include "ANet/ANet.h"
#include <winsock2.h>
#include <JuceHeader.h>
#include <future>

ANetPacket::ANetPacket(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const uint8_t* data, uint16_t len)
{
    ip.ip_src = src_ip;
    ip.ip_dst = dst_ip;

    udp.udp_dst_port = dst_port;
    udp.udp_src_port = src_port;

    udp.udp_len = len;

    if (data != nullptr)
        memcpy(payload, data, len);
}

/*
-------   ANet  -------  Wifi  -------
|Node1| <------>|Node2|<------>|Node3|
-------         -------        -------
*/

/* C'tor for client side */
ANetClient::ANetClient(const char *dest_ip, const char *dest_port, bool isAthernet)
    : m_isAthernet(isAthernet)
{
    m_destIP = inet_addr(dest_ip);
    m_destPort = htons(atoi(dest_port));

    m_selfIP = inet_addr(Config::IP_ATHERNET);
    m_selfPort = htons(atoi(Config::PORT_ATHERNET));

    if (isAthernet)
        m_audioIO = std::make_unique<AudioIO>();
    else
        m_udpClient= std::make_unique<UDPClient>(dest_ip, dest_port);
}

void ANetClient::SendData(const uint8_t* data, int len)
{
    struct ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, data, (uint16_t)len);
    size_t macLen = sizeof(ANetIP) + sizeof(ANetUDP) + len;
    if (m_isAthernet)
        m_audioIO->SendData((const uint8_t *)&packet, macLen);
    else
        m_udpClient->SendPacket(packet);
}

void ANetClient::SendPing(uint32_t target)
{
    if (m_isAthernet)
    {
        struct ANetPacket packet(m_selfIP, target, m_selfPort, m_destPort, nullptr, 0);
        int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + sizeof(ANetPing);

        auto now = std::chrono::system_clock::now();
        m_audioIO->SendData((const uint8_t *)&packet, macLen);
        m_audioIO->RecvData((uint8_t *)&packet, macLen);
        auto recv = std::chrono::system_clock::now();

        struct ANetPing ping{};
        memcpy(&ping, packet.payload, sizeof(ANetPing));

        if (ping.success)
        {
            std::cout << "Reply from " << packet.ip.ip_src << ": time="
                      << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count()
                      << "ms\n";
        }
        else
            std::cout << "Ping timed out\n";
    }
}

void ANetClient::SendPing()
{
    if (!m_isAthernet)
    {
        struct ANetPacket packet(m_destIP, m_selfIP, m_destPort, m_selfPort, nullptr, 1);
        if (!m_icmpPing)
            m_icmpPing = std::make_unique<IcmpPing>(m_destIP);

        auto ping = reinterpret_cast<struct ANetPing *>(packet.payload);
        ping->success = m_icmpPing->IcmpSendPing();

        m_audioIO->SendData((const uint8_t *)&packet, Config::MACDATA_PER_FRAME);
    }
}

ANetServer::ANetServer(const char *open_port, bool isAthernet)
    : m_isAthernet(isAthernet)
{
    m_openPort = htons(atoi(open_port));

    if (m_isAthernet)
        m_audioIO = std::make_unique<AudioIO>();
    else
        m_udpServer = std::make_unique<UDPServer>(open_port);
}

int ANetServer::RecvData(uint8_t* out, int outlen)
{
    struct ANetPacket packet{};
    struct in_addr ip_addr{};
    int length = 0;
    
    if (m_isAthernet) 
        length = m_audioIO->RecvData((uint8_t *)&packet, Config::MACDATA_PER_FRAME);
    else 
        length = m_udpServer->RecvPacket(packet);
    
    ip_addr.s_addr = packet.ip.ip_src;

    std::cout << "Received UDP packet from " << inet_ntoa(ip_addr) << ":" 
            << ntohs(packet.udp.udp_src_port) << " with " << packet.udp.udp_len << " bytes.\n";
    memcpy(out, packet.payload, packet.udp.udp_len);

    return length;
}

ANetGateway::ANetGateway(const char* anet_port, const char* out_port)
    : m_udpServer(out_port)
{
    m_anetPort = htons(atoi(anet_port));
    m_outPort = htons(atoi(out_port));
}

void ANetGateway::StartForwarding()
{
    auto atn2int = std::async(std::launch::async, [this]() { return ATNToInternet(); });
    auto int2atn = std::async(std::launch::async, [this]() { return InternetToATN(); });
    atn2int.get();
    int2atn.get();
}

void ANetGateway::ATNToInternet() {
    struct ANetPacket packet{};
    while (true)
    {
        m_audioIO.RecvData((uint8_t *)&packet, Config::MACDATA_PER_FRAME);
        uint32_t targetIP = packet.ip.ip_dst;
        if (m_NATTable.count(targetIP) == 0)
        {
            struct in_addr ipaddr{};
            ipaddr.S_un.S_addr = targetIP;

            m_NATTable[targetIP] = std::make_unique<ANetClient>(
                    inet_ntoa(ipaddr),
                    std::to_string(ntohs(m_outPort)).c_str(),
                    false);
        }
        switch (packet.ip.tag)
        {
        case ANetIPTag::PING:
            m_NATTable[targetIP]->SendPing();
            break;
        case ANetIPTag::DATA:
            m_NATTable[targetIP]->SendData((const uint8_t *)packet.payload, packet.udp.udp_len);
            break;
        }
        break;
    }
}

void ANetGateway::InternetToATN() {
    struct ANetPacket packet{};
    while (true)
    {
        m_udpServer.RecvPacket(packet);

        size_t macLen = packet.udp.udp_len;
        macLen += (sizeof(ANetIP) + sizeof(ANetUDP));
        m_audioIO.SendData((const uint8_t *)&packet, macLen);

        break;
    }
}
