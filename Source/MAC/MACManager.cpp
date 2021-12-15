#include "MAC/MACManager.h"

#include <memory>

MACManager& MACManager::get()
{
    static MACManager instance;
    return instance;
}

MACManager::MACManager()
{
    macFrameFactory = std::make_unique<MACFrameFactory>();
}

void MACManager::initialize(std::unique_ptr<MACLayerReceiver> &&macReceiver, 
                        std::unique_ptr<MACLayerTransmitter> &&macTransmitter,
                        std::unique_ptr<CSMASenderQueue> &&csmaSenderQueue)
{
    get().macReceiver = std::move(macReceiver);
    get().macTransmitter = std::move(macTransmitter);
    get().csmaSenderQueue = std::move(csmaSenderQueue);
}

void MACManager::destroy()
{
    if (get().macReceiver) {
        get().macReceiver->stopMACThread();
        get().macReceiver.reset();
    }

    if (get().macTransmitter) {
        get().macTransmitter->stopMACThread();
        get().macTransmitter.reset();
    }

    if (get().csmaSenderQueue) {
        get().csmaSenderQueue.reset();
    }

    if (get().macFrameFactory) {
        get().macFrameFactory = std::make_unique<MACFrameFactory>();
    }
}