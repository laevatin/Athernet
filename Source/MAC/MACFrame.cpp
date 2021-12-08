#include "MAC/MACFrame.h"
#include "Utils/CRC.h"

MACFrameFactory::MACFrameFactory()
    : m_nextid(0),
      m_crcTable(CRC::CRC_16_ARC())
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
    frame->header.id = m_nextid++;
    /* Set to zero at first, then calculate it. */
    frame->header.crc16 = 0;

    memcpy(frame->data, pdata + start, (size_t)len * sizeof(uint8_t));
    frame->header.crc16 = CRC::Calculate(frame, Config::MACHEADER_LENGTH / 8 + len, m_crcTable);
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
    frame->header.crc16 = 0;
    frame->header.crc16 = CRC::Calculate(frame, Config::BIT_PER_ACK / 8, m_crcTable);
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

bool MACFrameFactory::checkCRC(MACFrame *frame)
{
    uint16_t crcSaved = frame->header.crc16;
    frame->header.crc16 = 0;

    if (frame->header.type == Config::ACK) 
    {
        if (CRC::Calculate(frame, Config::BIT_PER_ACK / 8, m_crcTable) != crcSaved)
        {
            return false;
        }
    }
    else if (frame->header.type == Config::DATA)
    {
        if (CRC::Calculate(frame, Config::MACHEADER_LENGTH / 8 + frame->header.len, m_crcTable) != crcSaved)
        {
            return false;
        }
    }
    else 
    {
        return false;
    }

    frame->header.crc16 = crcSaved;
    return true;
}
