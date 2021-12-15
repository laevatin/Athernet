#pragma once

#ifndef __MACMANAGER_H__
#define __MACMANAGER_H__

#include <memory>
#include "MACLayer.h"

class MACManager
{
public:
    static void initialize(std::unique_ptr<MACLayerReceiver> &&macReceiver, 
                        std::unique_ptr<MACLayerTransmitter> &&macTransmitter,
                        std::unique_ptr<CSMASenderQueue> &&csmaSenderQueue);
    static MACManager& get();
    static void destroy();

    MACManager(MACManager const&) = delete;             // Copy construct
    MACManager(MACManager&&) = delete;                  // Move construct
    MACManager& operator=(MACManager const&) = delete;  // Copy assign
    MACManager& operator=(MACManager &&) = delete;      // Move assign
    
    std::unique_ptr<MACLayerReceiver> macReceiver;
    std::unique_ptr<MACLayerTransmitter> macTransmitter;
    std::unique_ptr<CSMASenderQueue> csmaSenderQueue; 
    std::unique_ptr<MACFrameFactory> macFrameFactory;

private:
    MACManager();
    ~MACManager() = default;
};

#endif