#pragma once

#include <string>
#include <cstdint>

struct OpcodeInfo
{
private:
    uint32_t m_opcode;

    std::string m_name;

public:
    OpcodeInfo(uint32_t opcode, const std::string& name);

    uint32_t getOpcode() const;

    const std::string& getName() const;
};
