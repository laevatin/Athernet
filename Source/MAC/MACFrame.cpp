#include "MAC/MACFrame.h"

MACFrameFactory::MACFrameFactory()
    : nextid(0)
{}

MACFrameFactory::~MACFrameFactory()
{}

MACFrame *MACFrameFactory::createDataFrame(const DataType &data, int start, int len)
{
    jassert(len <= Config::MACDATA_PER_FRAME);

    MACFrame *frame = new MACFrame();
    const uint8_t *pdata = data.getRawDataPointer();

    frame->header.dest = Config::RECEIVER;
    frame->header.src = Config::SENDER;
    frame->header.id = nextid++;

    memcpy(frame->data, pdata, (size_t)len);
    return frame;
}

MACFrame *MACFrameFactory::createACKFrame(uint8_t id)
{
    MACFrame *frame = new MACFrame();
    frame->header.dest = Config::SENDER;
    frame->header.src = Config::RECEIVER;
    frame->header.id = id;
    frame->header.len = 0;
    return frame;
}

MACFrame *MACFrameFactory::createEmpty()
{
    MACFrame *frame = new MACFrame();
    return frame;
}

void MACFrameFactory::destoryFrame(MACFrame *frame)
{
    delete frame;
}
