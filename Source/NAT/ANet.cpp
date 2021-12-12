#include "NAT/ANet.h"
#include <winsock2.h>

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

ANet::ANet(const char* self_ip, const char* self_port, const char* dest_ip, const char* dest_port, int node)
    : m_node(node)
{
    m_selfIP = inet_addr(self_ip);
    m_destIP = inet_addr(dest_ip);
    m_selfPort = htons(atoi(self_port));
    m_destPort = htons(atoi(dest_port));

    if (node == 1) 
    {
        m_audioIO.reset(new AudioIO());
    } 
    else if (node == 2)
    {
        m_udpServer.reset(new UDPServer(self_port));
        m_udpClient.reset(new UDPClient(dest_ip, dest_port));
        m_audioIO.reset(new AudioIO());
    }
    else if (node == 3)
    {
        m_udpClient.reset(new UDPClient(dest_ip, dest_port));
        m_udpServer.reset(new UDPServer(self_port));
    }
}

ANet::~ANet()
{
    m_udpClient.reset();
    m_udpServer.reset();
    m_audioIO.reset();
}

void ANet::SendData(const uint8_t* data, int len, int dest_node)
{
    if ((m_node == 1 && dest_node == 2) || (m_node == 2 && dest_node == 1)) 
    {
        struct ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, data, (uint16_t)len);
        int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + len;
        m_audioIO->SendData((const uint8_t *)&packet, macLen);
    }
    else if ((m_node == 2 && dest_node == 3) || (m_node == 3 && dest_node == 2))
    {
        m_udpClient->SendData(data, len);
    }
}

int ANet::RecvData(uint8_t* out, int outlen, int from_node)
{
    if ((m_node == 1 && from_node == 2) || (m_node == 2 && from_node == 1)) 
    {
        struct ANetPacket packet;
        struct in_addr ip_addr;
        int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + outlen;
        m_audioIO->RecvData((uint8_t *)&packet, macLen);
        ip_addr.s_addr = packet.ip.ip_src;

        std::cout << "Received UDP packet from " << inet_ntoa(ip_addr) << ":" 
                << ntohs(packet.udp.udp_src_port) << " with " << packet.udp.udp_len << " bytes.\n";
        memcpy(out, packet.payload, packet.udp.udp_len);
        return packet.udp.udp_len;
    }
    else if ((m_node == 2 && from_node == 3) || (m_node == 3 && from_node == 2))
    {
        return m_udpServer->RecvData(out, outlen);
    }
    return -1;
}

