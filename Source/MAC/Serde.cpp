#include "MAC/Serde.h"

#include "Physical/Frame.h"
#include "MAC/ACK.h"
#include "MAC/MACFrame.h"

uint8_t* serialize(MACFrame* frame)
{
    return reinterpret_cast<uint8_t*>(frame);
}

MACFrame* deserialize(uint8_t* data)
{
    return reinterpret_cast<MACFrame*>(data);
}

Frame convertFrame(MACFrame* macframe)
{
    uint8_t* serialized = serialize(macframe);
    if (macframe->header.type == Config::ACK)
        return ACK(serialized);
    else if (macframe->header.type == Config::DATA)
        return Frame(serialized);
    else
    {
        std::cerr << "convertFrame: Unknown frame type. \n";
        return Frame();
    }
}

void convertMACFrame(const Frame& phy, MACFrame* macFrame)
{
    uint8_t* serialized = serialize(macFrame);
    phy.getData(serialized);
}

MACHeader* headerView(uint8_t* data)
{
    return reinterpret_cast<MACHeader*>(data);
}


