#include "MAC/MACManager.h"

MACManager& MACManager::get()
{
    static MACManager instance;
    return instance;
}

MACManager::MACManager()
{
    macFrameFactory.reset(new MACFrameFactory());
}

MACManager::~MACManager()
{}

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
    if (get().macReceiver.get()) {
        get().macReceiver->stopMACThread();
        get().macReceiver.reset();
    }

    if (get().macTransmitter.get()) {
        get().macTransmitter->stopMACThread();
        get().macTransmitter.reset();
    }

    if (get().csmaSenderQueue.get()) {
        get().csmaSenderQueue.reset();
    }

    if (get().macFrameFactory.get()) {
        get().macFrameFactory.reset(new MACFrameFactory());
    }
}