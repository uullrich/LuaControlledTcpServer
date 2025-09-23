#include "CanMessage.h"

#include <sstream>
#include <iomanip>

CanMessage::CanMessage() 
    : m_id(0), m_data(), m_extended(false), m_rtr(false) {
}

CanMessage::CanMessage(uint32_t id, const std::vector<uint8_t>& data, bool extended, bool rtr) 
    : m_id(id), m_data(data), m_extended(extended), m_rtr(rtr) {
}

uint32_t CanMessage::getID() const {
    return m_id;
}

void CanMessage::setID(uint32_t id) {
    m_id = id;
}

std::vector<uint8_t> CanMessage::getData() const {
    return m_data;
}

void CanMessage::setData(const std::vector<uint8_t>& data) {
    m_data = data;
}

bool CanMessage::isExtended() const {
    return m_extended;
}

void CanMessage::setExtended(bool extended) {
    m_extended = extended;
}

bool CanMessage::isRTR() const {
    return m_rtr;
}

void CanMessage::setRTR(bool rtr) {
    m_rtr = rtr;
}

std::vector<uint8_t> CanMessage::serialize() const {
    std::vector<uint8_t> result;
    
    // Format:
    // - 4 bytes for ID
    // - 1 byte for flags (bit 0: extended, bit 1: rtr)
    // - 1 byte for data length
    // - N bytes for data

    // ID (4 bytes)
    result.push_back(static_cast<uint8_t>((m_id >> 24) & 0xFF));
    result.push_back(static_cast<uint8_t>((m_id >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((m_id >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(m_id & 0xFF));
    
    // Flags
    uint8_t flags = 0;
    if (m_extended) flags |= 0x01;
    if (m_rtr) flags |= 0x02;
    result.push_back(flags);
    
    // Data length
    result.push_back(static_cast<uint8_t>(m_data.size()));
    
    // Data
    result.insert(result.end(), m_data.begin(), m_data.end());
    
    return result;
}

CanMessage CanMessage::deserialize(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 6) {
        // Not enough data for a valid message
        return CanMessage();
    }
    
    // Extract ID (4 bytes)
    uint32_t id = static_cast<uint32_t>(bytes[0]) << 24 |
                  static_cast<uint32_t>(bytes[1]) << 16 |
                  static_cast<uint32_t>(bytes[2]) << 8 |
                  static_cast<uint32_t>(bytes[3]);
    
    // Extract flags
    bool extended = (bytes[4] & 0x01) != 0;
    bool rtr = (bytes[4] & 0x02) != 0;
    
    // Extract data length
    uint8_t dataLength = bytes[5];
    
    // Ensure we have enough bytes for the data
    if (bytes.size() < 6 + dataLength) {
        return CanMessage();
    }
    
    // Extract data
    std::vector<uint8_t> data(bytes.begin() + 6, bytes.begin() + 6 + dataLength);
    
    return CanMessage(id, data, extended, rtr);
}

std::string CanMessage::toString() const {
    std::stringstream stringSteam;
    stringSteam << "CAN [" << std::hex << std::uppercase << m_id << std::nouppercase << std::dec 
       << (m_extended ? " EXT" : " STD") 
       << (m_rtr ? " RTR" : "") 
       << "] ";
    
    for (auto byte : m_data) {
        stringSteam << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
    }
    
    return stringSteam.str();
}