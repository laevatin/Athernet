#include "UDP/UDP.h"
#include "ANet/ANet.h"
#include <iostream>

UDP::UDP(const char *port) {
    WORD socketVer = MAKEWORD(2, 2);
    if (WSAStartup(socketVer, &m_wsaData) != 0) {
        std::cerr << "Cannot initialize socket, exiting.\n";
        exit(-1);
    }
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_port = htons(atoi(port));
    m_sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;
}

UDP::~UDP() {
    WSACleanup();
    closesocket(m_socket);
}

UDPServer::UDPServer(const char *port)
        : UDP(port) {
    if (bind(m_socket, (sockaddr *) &m_sockaddr, sizeof(m_sockaddr)) == SOCKET_ERROR) {
        std::cerr << "Cannot bind to port " << ntohs(m_sockaddr.sin_port)
                  << ", exiting.\n";
        closesocket(m_socket);
        exit(-1);
    }
}

int UDPServer::RecvPacket(ANetPacket &out) {
    int remote_size = sizeof(sockaddr);
    int data_size = recvfrom(m_socket, (char *) out.payload, Config::IP_PACKET_PAYLOAD, 0, (sockaddr *) &m_sockaddr_remote,
                             &remote_size);

    out.ip.ip_src = m_sockaddr_remote.sin_addr.S_un.S_addr;
    out.udp.udp_src_port = m_sockaddr_remote.sin_port;
    out.udp.udp_len = data_size;

    return data_size;
}

UDPClient::UDPClient(const char *ip, const char *port)
        : UDP(port) {
    unsigned long ipaddr = inet_addr(ip);
    m_sockaddr.sin_addr.S_un.S_addr = ipaddr;
}

void UDPClient::SendPacket(const ANetPacket &packet) {
    int len = packet.udp.udp_len;

    sendto(m_socket, (char *) packet.payload, len, 0, (sockaddr *) &m_sockaddr, sizeof(m_sockaddr));
}