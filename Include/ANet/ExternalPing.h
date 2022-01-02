#pragma once

#ifndef __EXTERNAL_PING_H__
#define __EXTERNAL_PING_H__

#include <IPv4Layer.h>
#include <Packet.h>
#include <PcapFileDevice.h>
#include <IcmpLayer.h>
#include <IcmpAPI.h>
#include <EthLayer.h>
#include "PcapDevice.h"
#include "PcapLiveDeviceList.h"
#include "SystemUtils.h"

#include <functional>

class PingCapture {
public:
    PingCapture(const std::string &ipAddr);
    ~PingCapture();

    inline bool isWorking() { return m_isWorking; };
    void startCapture(std::function<void(void)> callback);
    void stopCapture();

private:

    static void replyPing(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* callback);

    pcpp::PcapLiveDevice *m_device;
    bool m_isWorking = true;
};

#endif