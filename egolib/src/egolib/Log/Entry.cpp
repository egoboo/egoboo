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
const Manipulations::EndOfEntryManipulation EndOfEntry{};
const Manipulations::EndOfLineManipulation EndOfLine{};

Entry::Entry(Level level)
    : m_level(level), m_sink()
{}

Entry::Entry(Level level, const std::string& fileName, int lineNumber)
    : Entry(level)
{
    m_attributes["C/C++ file name"] = fileName;
    m_attributes["C/C++ line number"] = std::to_string(lineNumber);
}

Entry::Entry(Level level, const std::string& fileName, int lineNumber, const std::string& functionName)
    : Entry(level, fileName, lineNumber)
{
    m_attributes["C/C++ function name"] = functionName;
}

bool Entry::hasAttribute(const std::string& name) const
{
    return m_attributes.cend() != m_attributes.find(name);
}

const std::string& Entry::getAttribute(const std::string& name) const
{
    return m_attributes.find(name)->second;
}

std::string Entry::getText() const
{
    return m_sink.str();
}

Level Entry::getLevel() const
{
    return m_level;
}

Entry& operator<<(Entry& entry, const char *value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, const std::string& value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, char value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed char value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned char value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed short value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned short value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, bool value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed int value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned int value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, signed long value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned long value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, float value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, double value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, const Manipulations::EndOfLineManipulation& value)
{
    entry.m_sink << std::endl;
    return entry;
}

Entry& operator<<(Entry& entry, const Manipulations::EndOfEntryManipulation& value)
{
    entry.m_sink << std::endl;
    return entry;
}

Entry& operator<<(Entry& entry, signed long long value)
{
    entry.m_sink << value;
    return entry;
}

Entry& operator<<(Entry& entry, unsigned long long value)
{
    entry.m_sink << value;
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
