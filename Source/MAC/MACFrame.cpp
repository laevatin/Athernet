#include "MAC/MACFrame.h"
#include "Utils/CRC.h"

uint8_t MACFrame::nextID = 0;
CRC::Table<std::uint16_t, 16> MACFrame::crcTable(CRC::CRC_16_ARC());

MACFrame::MACFrame(uint8_t id, MACType type)
    : m_macHeader() {
    m_macHeader.dest  = Config::OTHER;
    m_macHeader.src   = Config::SELF;
    m_macHeader.type  = (uint8_t)type;
    m_macHeader.len   = 0;
    m_macHeader.id    = id;
    m_macHeader.crc16 = 0;
    m_macHeader.crc16 = CRC::Calculate(&m_macHeader, sizeof(MACHeader), crcTable);
    m_isGoodMACFrame  = true;
}

MACFrame::MACFrame(const void *mac_payload, uint16_t length)
    : m_macHeader() {
    jassert(length <= Config::MACDATA_PER_FRAME);

    m_macHeader.dest  = Config::OTHER;
    m_macHeader.src   = Config::SELF;
    m_macHeader.type  = (uint8_t)MACType::DATA;
    m_macHeader.len   = length;
    m_macHeader.id    = nextID++;
    m_macHeader.crc16 = 0;
    m_payload.addArray(mac_payload, length);
    uint16_t crcHeader = CRC::Calculate(&m_macHeader, sizeof(MACHeader), crcTable);
    m_macHeader.crc16 = CRC::Calculate(m_payload.getRawDataPointer(), length, crcTable, crcHeader);
    m_isGoodMACFrame  = true;
}

MACFrame::MACFrame(const Frame &frame)
    : m_macHeader() {
    DataType rawData;
    frame.getData(rawData);
    if (rawData.size() < sizeof(MACHeader)) {
        m_isGoodMACFrame = false;
#ifdef DEBUG
        std::cout << __FILE__ << "wrong frame size\n";
#endif
        return;
    }

    auto dataPtr = rawData.getRawDataPointer();
    auto macHeader = reinterpret_cast<MACHeader *>(dataPtr);
    if (macHeader->len > rawData.size() - sizeof(MACHeader)) {
        m_isGoodMACFrame = false;
#ifdef DEBUG
        std::cout << __FILE__ << "wrong frame size " << rawData.size()
                  << " or MAC header length " << macHeader->len << "\n";
#endif
        return;
    }
    dataPtr += sizeof(MACHeader);
    m_macHeader = *macHeader;
    m_payload.addArray(dataPtr, macHeader->len);
    m_isGoodMACFrame = checkCRC();
}

MACFrame::MACFrame(MACFrame &&other) noexcept
    : m_macHeader(other.m_macHeader),
    m_payload(std::move(other.m_payload)),
    m_isGoodMACFrame(std::exchange(other.m_isGoodMACFrame, false)) {}

bool MACFrame::checkCRC() {
    uint16_t savedCRC = m_macHeader.crc16;
    m_macHeader.crc16 = 0;
    uint16_t crcHeader = CRC::Calculate(&m_macHeader, sizeof(MACHeader), crcTable);
    m_macHeader.crc16 = CRC::Calculate(m_payload.getRawDataPointer(), m_macHeader.len, crcTable, crcHeader);
    if (m_macHeader.crc16 != savedCRC) {
#ifdef DEBUG
        std::cout << "CRC Check failed for id: " << (int)m_macHeader.id << "\n";
#endif
        m_macHeader.crc16 = savedCRC;
        return false;
    }
    return true;
}

uint16_t MACFrame::serialize(void *out) const {
    uint16_t total_size = sizeof(MACHeader);
    auto *ptr = (uint8_t *)out;
    memcpy(out, &m_macHeader, sizeof(MACHeader));
    ptr += sizeof(MACHeader);
    memcpy(ptr, m_payload.getRawDataPointer(), m_macHeader.len);
    total_size += m_macHeader.len;
    return total_size;
}

bool MACFrame::isGood() const {
    return m_isGoodMACFrame;
}

const MACHeader &MACFrame::getHeader() const {
    return m_macHeader;
}
