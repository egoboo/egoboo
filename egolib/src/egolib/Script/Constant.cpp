#include "egolib/Script/Constant.hpp"

namespace Ego
{
namespace Script
{
	
Constant::Constant()
	: m_kind(Kind::Void), m_integer(0)
{}

Constant::Constant(const Constant& other)
    : m_kind(other.m_kind)
{
    switch (m_kind)
    {
        case Constant::Kind::Integer:
            m_integer = other.m_integer;
            break;
        case Constant::Kind::Void:
            break;
    };
}

Constant::Constant(Constant&& other)
    : m_kind(other.m_kind)
{
    switch (m_kind)
    {
        case Constant::Kind::Integer:
            m_integer = other.m_integer;
            break;
        case Constant::Kind::Void:
            break;
    };
}

Constant::Constant(int value)
	: m_kind(Kind::Integer), m_integer(value)
{}

Constant& Constant::operator=(Constant other)
{
    swap(*this, other);
    return *this;
}

bool Constant::operator==(const Constant& other) const
{
    if (m_kind != other.m_kind) return false;
    switch (m_kind)
    {
        case Constant::Kind::Integer:
            return m_integer == other.m_integer;
        case Constant::Kind::Void:
            return true;
    };
}
	
} // namespace Script
} // namespace Ego