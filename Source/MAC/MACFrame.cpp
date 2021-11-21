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

    frame->header.dest = Config::OTHER;
    frame->header.src = Config::SELF;
    frame->header.type = Config::DATA;
    frame->header.len = len;
    frame->header.id = nextid++;

    memcpy(frame->data, pdata + start, (size_t)len * sizeof(uint8_t));
    return frame;
}

MACFrame *MACFrameFactory::createACKFrame(uint8_t id)
{
    MACFrame *frame = new MACFrame();
    frame->header.dest = Config::OTHER;
    frame->header.src = Config::SELF;
    frame->header.type = Config::ACK;
    frame->header.id = id;
    frame->header.len = 0;
    return frame;
}

MACFrame* MACFrameFactory::createPingReply(uint8_t id) {
	MACFrame* frame = new MACFrame();
	frame->header.dest = Config::OTHER;
	frame->header.src = Config::SELF;
	frame->header.type = Config::MACPING_REPLY;
	frame->header.id = id;
	frame->header.len = 0;
	return frame;
}

MACFrame* MACFrameFactory::createPingReq(uint8_t id) {
	MACFrame* frame = new MACFrame();
	frame->header.dest = Config::OTHER;
	frame->header.src = Config::SELF;
	frame->header.type = Config::MACPING_REQ;
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
