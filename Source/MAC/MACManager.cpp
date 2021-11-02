#include "MAC/MACManager.h"

MACManager& MACManager::get()
{
    static MACManager instance;
    return instance;
}

MACManager::MACManager()
{}

MACManager::~MACManager()
{}

void MACManager::initialize(std::unique_ptr<MACLayerReceiver> &&macReceiver, 
                        std::unique_ptr<MACLayerTransmitter> &&macTransmitter)
{
    get().macReceiver = std::move(macReceiver);
    get().macTransmitter = std::move(macTransmitter);
}

void MACManager::destroy()
{
    if (get().macReceiver.get()) {
        get().macReceiver->stopMACThread();
        get().macReceiver.release();
    }

    if (get().macTransmitter.get()) {
        get().macTransmitter->stopMACThread();
        get().macTransmitter.release();
    }
}