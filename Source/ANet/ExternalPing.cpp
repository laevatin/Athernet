#include "ANet/ExternalPing.h"
#include <iostream>
#include "Config.h"

PingCapture::PingCapture(const std::string &ipAddr) {
    m_device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(pcpp::IPv4Address(ipAddr));

    if (!m_device) {
        m_isWorking = false;
        return;
    }

    if (!m_device->open()) {
        m_isWorking = false;
        return;
    }

    pcpp::ProtoFilter protocolFilter(pcpp::ICMP);
    m_device->setFilter(protocolFilter);
}

void PingCapture::startCapture(void *callback) {
    std::cout << "Capture started..." << std::endl;
    if (m_device == nullptr) {
        std::cout << "Capture device not set up correctly.\n";
        return;
    }
    m_device->startCapture(replyPing, callback);
}

void PingCapture::stopCapture() {
    m_device->stopCapture();
}

void PingCapture::replyPing(pcpp::RawPacket *rawPacket, pcpp::PcapLiveDevice *dev, void *callback) {
    pcpp::Packet received(rawPacket);
    auto srcIP = received.getLayerOfType<pcpp::IPv4Layer>()->getSrcIPv4Address();
    auto srcIP_str = srcIP.toString();
    std::cout << "Received ping from " << srcIP_str << "\n";

    if (!received.isPacketOfType(pcpp::ICMP) || !received.isPacketOfType(pcpp::IPv4) || !srcIP_str.compare(Config::IP_ETHERNET))
        return;

    std::function<void(const char *)> resendPing((void(*)(const char*))callback);
    
    resendPing(srcIP_str.c_str());

    pcpp::Packet packet(received);

    /* Set Mac address */
    auto eth = packet.getLayerOfType<pcpp::EthLayer>();
    eth->setDestMac(received.getLayerOfType<pcpp::EthLayer>()->getSourceMac());
    eth->setSourceMac(dev->getMacAddress());

    /* Set IP address */
    auto ip = packet.getLayerOfType<pcpp::IPv4Layer>();
    ip->setDstIPv4Address(srcIP);
    ip->setSrcIPv4Address(dev->getIPv4Address());

    /* Set ICMP type to reply */
    auto icmp = packet.getLayerOfType<pcpp::IcmpLayer>();
    icmp->getIcmpHeader()->type = 0;

    packet.computeCalculateFields();

    bool success = dev->sendPacket(&packet);

    std::cout << packet.toString();
    if (success)
        std::cout << "reply success\n";
    else
        std::cout << "reply failed\n";
}

PingCapture::~PingCapture() {
    m_device->close();
}
