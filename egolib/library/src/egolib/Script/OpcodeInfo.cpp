#include "egolib/Script/OpcodeInfo.hpp"

OpcodeInfo::OpcodeInfo(uint32_t opcode, const std::string& name)
    : m_opcode(opcode), m_name(name)
{}

uint32_t OpcodeInfo::getOpcode() const
{
    return m_opcode;
}

const std::string& OpcodeInfo::getName() const
{
    return m_name;
}
