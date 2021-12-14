#include "ANet/ANet.h"
#include <winsock2.h>
#include <JuceHeader.h>

ANetPing::ANetPing(const char *IP)
{
    strcpy_s(destIP, IP);
}

ANetPacket::ANetPacket(uint32_t srcip, uint32_t dstip, uint16_t srcport, uint16_t dstport, const uint8_t* data, uint16_t len)
{
    ip.ip_src = srcip;
    ip.ip_dst = dstip;

    udp.udp_dst_port = dstport;
    udp.udp_src_port = srcport;

    udp.udp_len = len;

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
    m_destIP = htons(atoi(Config::PORT_ATHERNET));

    if (isAthernet)
        m_audioIO.reset(new AudioIO());
    else 
        m_udpClient.reset(new UDPClient(dest_ip, dest_port));

    // if (node == 1) 
    // {
    //     m_audioIO.reset(new AudioIO());
    // } 
    // else if (node == 2)
    // {
    //     m_udpServer.reset(new UDPServer(self_port));
    //     m_udpClient.reset(new UDPClient(dest_ip, dest_port));
    //     m_audioIO.reset(new AudioIO());
    // }
    // else if (node == 3)
    // {
    //     m_udpClient.reset(new UDPClient(dest_ip, dest_port));
    //     m_udpServer.reset(new UDPServer(self_port));
    // }
}

void ANetClient::SendData(const uint8_t* data, int len, int dest_node)
{
    struct ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, data, (uint16_t)len);
    int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + len;
    if (m_isAthernet)
        m_audioIO->SendData((const uint8_t *)&packet, macLen);
    else
        m_udpClient->SendPacket(packet);
}


// void ANetClient::Gateway(int from, int to)
// {
//     uint32_t ip;
//     uint16_t port;
//     int recv = 0;
//     struct ANetPacket packet;
//     struct in_addr ip_addr;

//     if (from == 3)
//         recv = m_udpServer->RecvData((uint8_t*)packet.payload, Config::MACDATA_PER_FRAME, &ip, &port);
//     else 
//     {
//         recv = m_audioIO->RecvData((uint8_t *)&packet, Config::MACDATA_PER_FRAME);

//         std::cout << packet.payload << "\n";
//         ip_addr.s_addr = packet.ip.ip_src;
//         std::cout << "Received UDP packet from " << inet_ntoa(ip_addr) << ":"
//             << ntohs(packet.udp.udp_src_port) << " with " << packet.udp.udp_len << " bytes.\n";
//     }

//     if (to == 3)
//         SendData((const uint8_t *)packet.payload, packet.udp.udp_len, 3);
//     else 
//     {
//         packet.ip.ip_src = ip;
//         packet.udp.udp_src_port = port;
//         packet.udp.udp_len = recv;
//         m_audioIO->SendData((const uint8_t *)&packet, Config::MACDATA_PER_FRAME);
//     }

// }

void ANetClient::SendPing(const char *target)
{
    struct ANetPing ping(target);
    int len = sizeof(ANetPing);
    struct ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, (const uint8_t *)&ping, len);
    int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + len;
    
    auto now = std::chrono::system_clock::now();
    m_audioIO->SendData((const uint8_t *)&packet, macLen);
    m_audioIO->RecvData((uint8_t *)&packet, macLen);
    auto recv = std::chrono::system_clock::now();

    memcpy(&ping, packet.payload, len);

    if (ping.success)
    {
        std::cout << "Reply from " << ping.destIP << ": time=" 
                    << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count() 
                    << "ms\n";
    }
    else 
        std::cout << "Ping timed out\n";
    
    // struct ANetPacket packet;
    // m_audioIO->RecvData((uint8_t *)&packet, Config::MACDATA_PER_FRAME);

    // struct ANetPing *ping = reinterpret_cast<struct ANetPing *>(packet.payload);
    // IcmpPing icmpPing(pingIP);

    // if (icmpPing.IcmpSendPing()) 
    //     ping->success = true;
    // else 
    //     ping->success = false;

    // m_audioIO->SendData((const uint8_t *)&packet, Config::MACDATA_PER_FRAME);

}

ANetServer::ANetServer(const char *open_port, bool isAthernet)
    : m_isAthernet(isAthernet)
{
    m_openPort = htons(atoi(open_port));

    if (m_isAthernet) 
        m_audioIO.reset(new AudioIO());
    else 
        m_udpServer.reset(new UDPServer(open_port));
}

int ANetServer::RecvData(uint8_t* out, int outlen)
{
    struct ANetPacket packet;
    struct in_addr ip_addr;
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
