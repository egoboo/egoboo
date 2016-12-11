#include "egolib/Script/ConstantPool.hpp"

namespace Ego {
namespace Script {

ConstantPool::ConstantPool()
{}

ConstantPool::ConstantPool(const ConstantPool& other)
    : m_constants(other.m_constants)
{}

ConstantPool::ConstantPool(ConstantPool&& other)
    : m_constants(std::move(other.m_constants))
{}

ConstantPool& ConstantPool::operator=(ConstantPool other)
{
    swap(*this, other);
    return *this;
}

const Constant& ConstantPool::getConstant(ConstantPool::Index index)
{
    if (index < 0 || index >= m_constants.size())
    {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
    }
    return m_constants[index];
}

ConstantPool::Size ConstantPool::getNumberOfConstants() const
{
    return (uint32_t)m_constants.size();
}

ConstantPool::Index ConstantPool::getOrCreateConstant(int value)
{
    // (1) Check if a constant for the specified value already exists.
    auto it = std::find_if(m_constants.cbegin(), m_constants.cend(),
                           [&value](const auto& constant) { return constant == value; });
    if (it == m_constants.cend())
    {
        if (m_constants.size() == std::numeric_limits<uint32_t>::max())
        {
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "constant pool overflow");
        }
        // (2) No constant for the specified value already exists. Create one.
        m_constants.push_back(Constant(value));
        it = m_constants.cend() - 1;
    }
    // (3) Return the index of the constant.
    return it - m_constants.begin();
}

void ConstantPool::clear()
{
    m_constants.clear();
}

} // namespace Script
} // namespace Ego
