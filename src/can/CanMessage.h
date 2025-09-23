#pragma once

#include <cstdint>
#include <vector>
#include <string>

class CanMessage {
public:
    CanMessage();
    CanMessage(uint32_t id, const std::vector<uint8_t>& data, bool extended = false, bool rtr = false);

    uint32_t getID() const;
    void setID(uint32_t id);

    std::vector<uint8_t> getData() const;
    void setData(const std::vector<uint8_t>& data);

    bool isExtended() const;
    void setExtended(bool extended);

    bool isRTR() const;
    void setRTR(bool rtr);

    // Convert to byte array for transmission
    std::vector<uint8_t> serialize() const;
    
    // Create from byte array
    static CanMessage deserialize(const std::vector<uint8_t>& bytes);
    
    // String representation for logging
    std::string toString() const;

private:
    uint32_t m_id;
    std::vector<uint8_t> m_data;
    bool m_extended;
    bool m_rtr;
};