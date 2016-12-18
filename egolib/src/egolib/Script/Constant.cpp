#include "egolib/Script/Constant.hpp"

namespace Ego
{
namespace Script
{
	
Constant::Constant()
	: m_kind(Kind::Void)
{}

Constant::Constant(int value)
    : m_kind(Kind::Integer), m_integer(value)
{}

Constant::Constant(const std::string& value)
    : m_kind(Kind::String), m_string(value)
{}

Constant::Constant(const Constant& other)
{
    m_kind = other.m_kind;
    switch (m_kind)
    {
        case Kind::Integer:
            m_integer = other.m_integer;
            break;
        case Kind::String:
            new (&m_string) auto(other.m_string);
            break;
        case Kind::Void:
            break;
    };
}

Constant::Constant(Constant&& other)
{
    m_kind = other.m_kind;
    switch (m_kind)
    {
        case Kind::Integer:
            m_integer = other.m_integer;
            break;
        case Kind::String:
            static_assert(std::is_nothrow_move_constructible<std::string>::value, "std::string is not nothrow move constructible");
            new (&m_string) auto(std::move(other.m_string));
            break;
        case Kind::Void:
            break;
    };
}

Constant::~Constant()
{
    switch (m_kind)
    {
        case Kind::Integer:
            break;
        case Kind::String:
            m_string.~basic_string();
            break;
        case Kind::Void:
            break;
    };
}

Constant& Constant::operator=(const Constant& other)
{
    if (this == &other)
    {
        return *this;
    }
    switch (other.m_kind)
    {
        case Kind::Integer:
            if (m_kind == Kind::String)
            {
                m_string.~basic_string();
            }
            m_integer = other.m_integer;
            break;
        case Kind::String:
            if (m_kind == Kind::String)
            {
                auto temporary(other.m_string);
                static_assert(std::is_nothrow_move_assignable<std::string>::value, "std::string is not nothrow move assignable");
                m_string = std::move(temporary);
            }
            else
            {
                auto temporary(other.m_string);
                static_assert(std::is_nothrow_move_constructible<std::string>::value, "std::string is not nothrow move constructible");
                new (&m_string) auto(std::move(temporary));
            }
            break;
        case Kind::Void:
            if (m_kind == Kind::String)
            {
                m_string.~basic_string();
            }
            break;
    };
    m_kind = other.m_kind;
    return *this;
}

bool Constant::operator==(const Constant& other) const
{
    if (m_kind != other.m_kind) return false;
    switch (m_kind)
    {
        case Kind::Integer:
            return m_integer == other.m_integer;
        case Kind::String:
            return m_string == other.m_string;
        case Kind::Void:
            return true;
    };
}
	
} // namespace Script
} // namespace Ego
