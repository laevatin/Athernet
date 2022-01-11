#include "ANet/ANet.h"
#include <winsock2.h>
#include <JuceHeader.h>
#include <future>
#include "ANet/ExternalPing.h"
#include <Ws2tcpip.h>
#include <tchar.h>

enum ANetIPTag {
    DATA,
    PING,
    START_PING,
    STOP_PING,
    START_RECV
};

ANetPacket::ANetPacket(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const uint8_t *data,
                       uint16_t len) {
    ip.ip_src = src_ip;
    ip.ip_dst = dst_ip;
    ip.tag = ANetIPTag::DATA;

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

/* C'tor for m_client side */
ANetClient::ANetClient(const char *dest_ip, const char *dest_port, bool isAthernet)
        : m_isAthernet(isAthernet) {
    InetPton(AF_INET, dest_ip, &m_destIP);
    InetPton(AF_INET, Config::IP_ATHERNET, &m_selfIP);

    m_destPort = htons(std::strtol(dest_port, nullptr, 10));
    m_selfPort = htons(std::strtol(Config::PORT_ATHERNET, nullptr, 10));

    if (isAthernet)
        m_audioIO = std::make_unique<AudioIO>();
    else
        m_udpClient = std::make_unique<UDPClient>(dest_ip, dest_port);
}

void ANetClient::SendData(const uint8_t *data, int len) {
    struct ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, data, (uint16_t) len);
    int macLen = len + sizeof(ANetIP) + sizeof(ANetUDP);
    if (m_isAthernet)
        AudioIO::SendData((const uint8_t *) &packet, macLen);
    else
        m_udpClient->SendPacket(packet);
}

void ANetClient::SendPing(const char *target, const char *src) {
    if (m_isAthernet) {
        uint32_t pingTarget, pingSrc;
        InetPton(AF_INET, target, &pingTarget);

        if (src != nullptr) {
            InetPton(AF_INET, src, &pingSrc);
        } else {
            pingSrc = m_selfIP;
        }

        struct ANetPacket packet(pingSrc, pingTarget, m_selfPort, m_destPort, nullptr, 1);
        packet.ip.tag = ANetIPTag::PING;

        int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + sizeof(ANetPing);

        auto now = std::chrono::system_clock::now();
        AudioIO::SendData((const uint8_t *) &packet, macLen);
        AudioIO::RecvData((uint8_t *) &packet, macLen);
        auto recv = std::chrono::system_clock::now();

        struct ANetPing ping{};
        memcpy(&ping, packet.payload, sizeof(ANetPing));

        char stringBuf[16];
        InetNtop(AF_INET, &packet.ip.ip_src, stringBuf, 16);

        if (ping.success) {
            std::cout << "Reply from " << stringBuf << ": time = "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(recv - now).count()
                      << "ms\n";
        } else
            std::cout << "Ping timed out.\n";
    }
}

void ANetClient::SendPing() {
    if (!m_isAthernet) {
        struct ANetPacket packet(m_destIP, m_selfIP, m_destPort, m_selfPort, nullptr, 1);
        if (!m_icmpPing)
            m_icmpPing = std::make_unique<IcmpPing>(m_destIP);

        auto ping = reinterpret_cast<struct ANetPing *>(packet.payload);
        ping->success = m_icmpPing->IcmpSendPing();
        int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + sizeof(ANetPing);

        AudioIO::SendData((const uint8_t *) &packet, macLen);
    }
}

void ANetClient::SendString(const std::string &str) {
    if (!m_isAthernet)
        return;

    constexpr int MAX_STR_LEN = Config::IP_PACKET_PAYLOAD - 1;

    int strPtr = 0;
    int strLen = (int) str.length() + 1;
    int sendLen = min(strLen, MAX_STR_LEN);

    while (strLen > MAX_STR_LEN) {
        ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, (const uint8_t *) str.c_str() + strPtr,
                          (uint16_t) sendLen);
        int macLen = sendLen + (int) sizeof(ANetIP) + (int) sizeof(ANetUDP);
        strLen -= MAX_STR_LEN;
        AudioIO::SendData((const uint8_t *) &packet, macLen);
        strPtr += MAX_STR_LEN;
    }

    sendLen = min(strLen, MAX_STR_LEN);
    ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, (const uint8_t *) str.c_str() + strPtr,
                      (uint16_t) sendLen);
    int macLen = sendLen + (int) sizeof(ANetIP) + (int) sizeof(ANetUDP);
    AudioIO::SendData((const uint8_t *) &packet, macLen);
}

void ANetClient::SendInt(int i) {
    if (!m_isAthernet)
        return;

    ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, (const uint8_t *) &i, sizeof(int));
    int macLen = sizeof(ANetIP) + sizeof(ANetUDP) + sizeof(int);
    AudioIO::SendData((const uint8_t *) &packet, macLen);
}

void ANetClient::StartPingReq() {
    if (!m_isAthernet)
        return;
    SendPacketWithType(ANetIPTag::START_PING);
}

void ANetClient::StopPingReq() {
    if (!m_isAthernet)
        return;

    SendPacketWithType(ANetIPTag::STOP_PING);
}

void ANetClient::StartRecvReq() {
    if (!m_isAthernet)
        return;

    SendPacketWithType(ANetIPTag::START_RECV);
}

void ANetClient::SendPacketWithType(ANetIPTag type) {
    if (!m_isAthernet)
        return;
    ANetPacket packet(m_selfIP, m_destIP, m_selfPort, m_destPort, nullptr, 0);
    packet.ip.tag = type;

    int macLen = sizeof(ANetIP) + sizeof(ANetUDP);
    AudioIO::SendData((const uint8_t *) &packet, macLen);
}

ANetServer::ANetServer(const char *open_port, bool isAthernet)
        : m_isAthernet(isAthernet) {
    m_openPort = htons(std::strtol(open_port, nullptr, 10));

    if (m_isAthernet)
        m_audioIO = std::make_unique<AudioIO>();
    else
        m_udpServer = std::make_unique<UDPServer>(open_port);
}

int ANetServer::RecvData(uint8_t *out, int outlen) {
    struct ANetPacket packet{};
    char stringBuf[16];

    if (m_isAthernet)
        AudioIO::RecvData((uint8_t *) &packet, Config::MACDATA_PER_FRAME);
    else
        m_udpServer->RecvPacket(packet);

    InetNtop(AF_INET, &packet.ip.ip_src, stringBuf, 16);

    std::cout << "Received UDP packet from " << stringBuf << ":"
              << ntohs(packet.udp.udp_src_port) << " with " << packet.udp.udp_len << " bytes.\n";
    memcpy(out, packet.payload, packet.udp.udp_len);

    return packet.udp.udp_len;
}

void ANetServer::ReplyPing() {
    ANetPacket packet;
    int length = AudioIO::RecvData((uint8_t *) &packet, Config::MACDATA_PER_FRAME);
    char stringBuf[16];
    InetNtop(AF_INET, &packet.ip.ip_src, stringBuf, 16);
    if (packet.ip.tag == ANetIPTag::PING) {
        std::cout << "Received ping from " << stringBuf << ".\n";
        auto ping = reinterpret_cast<struct ANetPing *>(packet.payload);
        ping->success = true;
        AudioIO::SendData((uint8_t *) &packet, length);
    }
}

void ANetServer::RecvString(std::string &str) {
    constexpr int MAX_STR_LEN = Config::IP_PACKET_PAYLOAD - 1;
    ANetPacket packet;
    str.clear();
    char *stringPtr = (char *) packet.payload;

    do {
        m_audioIO->RecvData((uint8_t *) &packet, Config::MACDATA_PER_FRAME);
        stringPtr[MAX_STR_LEN] = '\0';
        str.append(stringPtr);
    } while (strnlen_s(stringPtr, MAX_STR_LEN) == MAX_STR_LEN);
}

int ANetServer::RecvInt() {
    ANetPacket packet;
    int recvLen = sizeof(int) + (int) sizeof(ANetIP) + (int) sizeof(ANetUDP);
    m_audioIO->RecvData((uint8_t *) &packet, recvLen);
    return *(int *) packet.payload;
}

ANetGateway::ANetGateway(const char *anet_port, const char *out_port)
        : m_udpServer(out_port) {
    m_anetPort = htons(std::strtol(anet_port, nullptr, 10));
    m_outPort = htons(std::strtol(out_port, nullptr, 10));
}

int ANetGateway::replyCount = 0;

void ANetGateway::StartForwarding() {
//    auto int2atn = std::async(std::launch::async, [this]() { return InternetToATN(); });
    PingCapture capturer(Config::IP_ETHERNET);
    if (capturer.isWorking()) {
        capturer.startCapture((void *) ANetGateway::OnPingArrive);
    }

    while (true) {
        ANetPacket packet;
        if (replyCount > 0) {
            Sleep(200);
            continue;
        } else {
            AudioIO::RecvData((uint8_t *) &packet, Config::MACDATA_PER_FRAME);
        }
        switch (packet.ip.tag) {
            case ANetIPTag::START_PING: {
                std::cout << "Start pinging other devices.\n";
                ATNToInternet();
                break;
            }
            case ANetIPTag::START_RECV: {
                std::cout << "Start receiving ping from other devices.\n";
                replyCount = Config::PING_NUM;
                break;
            }
            default:
                std::cout << "Wrong state: " << (int) packet.ip.tag << "\n";
                break;
        }
    }
}

void ANetGateway::ATNToInternet() {
    struct ANetPacket packet{};
    while (true) {
        AudioIO::RecvData((uint8_t *) &packet, Config::MACDATA_PER_FRAME);
        uint32_t targetIP = packet.ip.ip_dst;
        if (m_NATTable.count(targetIP) == 0) {
            char stringBuf[16];
            InetNtop(AF_INET, &targetIP, stringBuf, 16);

            m_NATTable[targetIP] = std::make_unique<ANetClient>(
                    stringBuf, std::to_string(ntohs(m_outPort)).c_str(), false);
        }
#ifdef DEBUG
        std::cout << (char *) packet.payload << std::endl;
#endif
        switch (packet.ip.tag) {
            case ANetIPTag::PING:
                m_NATTable[targetIP]->SendPing();
                break;
            case ANetIPTag::DATA:
                m_NATTable[targetIP]->SendData((const uint8_t *) packet.payload, packet.udp.udp_len);
                break;
            case ANetIPTag::STOP_PING:
                return;
            default:
                break;
        }
    }
}

void ANetGateway::InternetToATN() {
    struct ANetPacket packet{};
    while (true) {
        m_udpServer.RecvPacket(packet);
        packet.udp.udp_dst_port = m_anetPort;

        int macLen = packet.udp.udp_len + sizeof(ANetIP) + sizeof(ANetUDP);
        AudioIO::SendData((const uint8_t *) &packet, macLen);
    }
}

void ANetGateway::OnPingArrive(const char *src_ip) {
    if (replyCount > 0) {
        ANetClient ANetConn(Config::IP_ATHERNET, Config::PORT_ATHERNET, true);
        ANetConn.SendPing(Config::IP_ATHERNET, src_ip);
        replyCount -= 1;
    }
}
