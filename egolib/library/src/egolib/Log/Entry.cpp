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
#include "idlib/idlib.hpp"
#include <map>
#include <memory>
#include <sstream>

namespace Log {

namespace Internal {

/// @brief An attribute.
class Attribute
{
private:
    /// @brief The name of this attribute.
    std::string m_name;
public:
    /// @brief Construct this attribute.
    /// @param name the name of this attribute
    Attribute(const std::string& name);

    /// @brief Destruct this attribute.
    virtual ~Attribute();

    /// @brief Get the name of this attribute.
    /// @return the name of this attribute
    const std::string& getName() const;
};

Attribute::Attribute(const std::string& name) :
    m_name(name)
{}

Attribute::~Attribute()
{}

const std::string& Attribute::getName() const
{
    return m_name;
}

/// @brief An attribute.
template <typename T>
class AttributeImpl : public Attribute
{
private:
    /// @brief The value of this attribute.
    T m_value;
public:
    /// @brief Construct this attribute.
    /// @param name the name of this attribute
    /// @param value the value of this attribute
    AttributeImpl(const std::string& name, const T& value) :
        Attribute(name), m_value(value)
    {}

    /// @brief Get the value of this attribute.
    /// @return the value of this attribute
    const T& getValue() const
    {
        return m_value;
    }
};

struct EntryImpl
{
private:
    Level m_level;
    std::map<std::string, std::unique_ptr<Attribute>> m_attributes;
public:
    /// @todo Make private, add accessor.
    std::ostringstream m_sink;
public:
    EntryImpl(const EntryImpl&) = delete;
    EntryImpl& operator=(const EntryImpl&) = delete;
	
public:
    /// @brief Construct the entry implementation.
    /// @param level the log level
    EntryImpl(Level level);

    /// @brief Get the log level.
	/// @return the log level
    Level getLevel() const;
    
	/// @brief Get the log text.
	/// @return the log text
    std::string getText() const;
	
	/// @brief Get the value of a string attribute.
	/// @param name the name of the string attribute
	/// @return the value of the string attribute if it is present, a null pointer otherwise
    /// @todo Make generic.
    AttributeImpl<std::string> *getAttribute(const std::string& name) const;

	/// @brief Get if a string attribute is present.
    /// @param name the name of the string attribute
	/// @return @a true if the string attribute is present, @a false otherwise
    /// @todo Make generic.
    bool hasAttribute(const std::string& name) const;

	/// @brief Set the value of a string attribute.
	/// @param name the name of the string attribute
	/// @param value the value of the string attribute
    /// @todo Make generic.
    void setAttribute(const std::string& name, const std::string& value);
};

EntryImpl::EntryImpl(Level level) :
	m_level(level)
{}

Level EntryImpl::getLevel() const
{
	return m_level;
}

std::string EntryImpl::getText() const
{
	return m_sink.str();
}

AttributeImpl<std::string> *EntryImpl::getAttribute(const std::string& name) const
{
	auto it = m_attributes.find(name);
	if (it == m_attributes.cend())
	{
        return nullptr;
	}
    return dynamic_cast<AttributeImpl<std::string> *>((*it).second.get());
}

bool EntryImpl::hasAttribute(const std::string& name) const
{
    return nullptr != getAttribute(name);
}

void EntryImpl::setAttribute(const std::string& name, const std::string& value)
{
	m_attributes[name] = std::make_unique<AttributeImpl<std::string>>(name, value);
}

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

bool Entry::hasAttribute(const std::string& name) const
{
    return impl->hasAttribute(name);
}

const std::string& Entry::getAttribute(const std::string& name) const
{
    auto *attribute = impl->getAttribute(name);
    if (nullptr == attribute)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "attribute `" + name + "` does not exist");
    }
    return attribute->getValue();
}

std::string Entry::getText() const
{
    return impl->getText();
}

Level Entry::getLevel() const
{
    return impl->getLevel();
}

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
