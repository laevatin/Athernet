#include "MAC/MACManager.h"

#include <memory>

MACManager &MACManager::get() {
    static MACManager instance;
    return instance;
}

void MACManager::initialize(std::unique_ptr<MACLayerReceiver> &&macReceiver,
                            std::unique_ptr<MACLayerTransmitter> &&macTransmitter) {
    get().macReceiver = std::move(macReceiver);
    get().macTransmitter = std::move(macTransmitter);
}

void MACManager::destroy() {
    if (get().macReceiver) {
        get().macReceiver.reset();
    }

    if (get().macTransmitter) {
        get().macTransmitter.reset();
    }
}