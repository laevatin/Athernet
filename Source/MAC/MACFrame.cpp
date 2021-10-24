#include "MAC/MACFrame.h"

MACFrameFactory::MACFrameFactory()
    : nextid(0)
{}

MACFrameFactory::~MACFrameFactory()
{}

MACFrame *MACFrameFactory::createDataFrame(const DataType &data, int start, int len)
{
    jassert(len <= Config::DATA_PER_FRAME / 8 - 5);

    MACFrame *frame = new MACFrame();
    frame->dest = RECEIVER;
    frame->src = SENDER;
    frame->id = nextid++;

    const uint8_t *pdata = data.getRawDataPointer();

    memcpy(frame->data, pdata, (size_t)len);
    return frame;
}

MACFrame *MACFrameFactory::createACKFrame(uint8_t id)
{
    MACFrame *frame = new MACFrame();
    frame->dest = SENDER;
    frame->src = RECEIVER;
    frame->id = id;
    frame->len = 0;
    return frame;
}

void destoryFrame(MACFrame *frame)
{
    delete frame;
}
