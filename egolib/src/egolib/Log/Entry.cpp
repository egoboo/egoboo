//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/Log/Entry.cpp
/// @brief A log entry

#include "egolib/Log/Entry.hpp"

namespace Log {

namespace Internal {

struct EntryImpl
{
    Level m_level;
    std::map<std::string, std::string> m_attributes;
    std::ostringstream m_sink;
public:
    EntryImpl(const EntryImpl&) = delete;
    EntryImpl& operator=(const EntryImpl&) = delete;
    EntryImpl(Level level) : m_level(level)
    {}

    /// get level
    Level getLevel() const { return m_level; }
    /// get text
    std::string getText() const { return m_sink.str(); }
    /// get the attribute value, raise if attribute is not present
    const std::string& getAttribute(const std::string& key)
    {
        auto it = m_attributes.find(key);
        if (it == m_attributes.cend())throw Id::RuntimeErrorException(__FILE__, __LINE__, "key not found");
        return (*it).second;
    }
    /// get if an attribute is present
    bool hasAttribute(const std::string& key) const { return m_attributes.find(key) != m_attributes.cend(); }
    /// set an attribute
    void setAttribute(const std::string& key, const std::string& value) { m_attributes[key] = value; }
};

} // namespace Internal

Entry::Entry(Level level)
    : impl(new Internal::EntryImpl(level))
{}

Entry::Entry(Level level, const std::string& fileName, int lineNumber)
    : Entry(level)
{
    impl->setAttribute("C/C++ file name", fileName);
    impl->setAttribute("C/C++ line number", std::to_string(lineNumber));
}

Entry::Entry(Level level, const std::string& fileName, int lineNumber, const std::string& functionName)
    : Entry(level, fileName, lineNumber)
{
    impl->setAttribute("C/C++ function name", functionName);
}

Entry::~Entry()
{
    if (nullptr != impl)
    {
        delete impl;
        impl = nullptr;
    }
}

Entry::Entry(Entry&& other) :
    impl(other.impl)
{
    other.impl = nullptr;
}

Entry& Entry::operator=(Entry&& other)
{
    if (this != &other)
    {
        impl = other.impl;
        other.impl = nullptr;
    }
    return *this;
}

bool Entry::hasAttribute(const std::string& name) const { return impl->hasAttribute(name); }

const std::string& Entry::getAttribute(const std::string& name) const { return impl->getAttribute(name); }

std::string Entry::getText() const { return impl->getText(); }

Level Entry::getLevel() const { return impl->getLevel(); }

Entry& operator<<(Entry& entry, const char *value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, const std::string& value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, char value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed char value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned char value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed short value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned short value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, bool value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed int value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned int value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed long value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned long value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, float value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, double value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, const Internal::EndOfLineManipulation& value)
{
    entry.impl->m_sink << std::endl;
    return entry;
}

Entry& operator<<(Entry& entry, const Internal::EndOfEntryManipulation& value)
{
    entry.impl->m_sink << std::endl;
    return entry;
}

Entry& operator<<(Entry& entry, signed long long value)
{
    entry.impl->m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned long long value)
{
    entry.impl->m_sink << value;
    return entry;
}

Target& operator<<(Target& target, const Entry& entry)
{
    std::ostringstream temporary;
    if (entry.hasAttribute("C/C++ file name"))
    {
        temporary << entry.getAttribute("C/C++ file name") << ":";
    }
    if (entry.hasAttribute("C/C++ line number"))
    {
        temporary << entry.getAttribute("C/C++ line number") << ":";
    }
    if (entry.hasAttribute("C/C++ function namer"))
    {
        temporary << entry.getAttribute("C/C++ function name") << ":";
    }
    target.log(entry.getLevel(), "%s%s", temporary.str().c_str(), entry.getText().c_str());
    return target;
}

} // namespace Log
