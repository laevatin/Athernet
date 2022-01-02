#include "ANet/ExternalPing.h"
#include <iostream>

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

void PingCapture::startCapture(std::function<void()> callback) {
    std::cout << "Capture started..." << std::endl;
    m_device->startCapture(replyPing, (void *)callback.target<void()>());
}

void PingCapture::stopCapture() {
    m_device->stopCapture();
}

void PingCapture::replyPing(pcpp::RawPacket *rawPacket, pcpp::PcapLiveDevice *dev, void *callback) {
    pcpp::Packet received(rawPacket);
    std::cout << "Received ping\n";

    if (!received.isPacketOfType(pcpp::ICMP) || !received.isPacketOfType(pcpp::IPv4) || !icmp->getEchoRequestData())
        return;

    pcpp::Packet packet(received);

    /* Set Mac address */
    auto eth = packet.getLayerOfType<pcpp::EthLayer>();
    eth->setDestMac(received.getLayerOfType<pcpp::EthLayer>()->getSourceMac());
    eth->setSourceMac(dev->getMacAddress());

    /* Set IP address */
    auto ip = packet.getLayerOfType<pcpp::IPv4Layer>();
    ip->setDstIPv4Address(received.getLayerOfType<pcpp::IPv4Layer>()->getSrcIPv4Address());
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
