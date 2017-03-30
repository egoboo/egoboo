#pragma once

#include "egolib/Script/EnumDescriptor.hpp"
#include "egolib/Script/Conversion.hpp"

namespace Ego {
namespace Configuration {

/// @brief The base of all variables.
template <typename ValueTypeArg>
class VariableBase : public Id::NonCopyable
{
public:
    using ValueType = ValueTypeArg;

private:
    /// @brief The default value of the variable.
    const ValueType m_defaultValue;

    /// @brief The partially qualified name of this element e.g. <tt>video.fullscreen</tt>.
    std::string m_name;

    /// @brief The description of the variable e.g. <tt>Enable/disable fullscreen mode.</tt>.
    std::string m_description;

    /// @brief The value of the variable.
    ValueType m_value;

protected:
    /// @brief Construct this variable.
    /// @param defaultValue the default value
    /// @param name the qualified name of the variable
    /// @param description the description of the variable
    VariableBase(const ValueType& defaultValue, const std::string& name, const std::string& description) :
        m_defaultValue(defaultValue), m_name(name), m_description(description), m_value(defaultValue)
    {}

    /// @brief Destruct this variable.
    virtual ~VariableBase() {}

public:

    /// @brief Get the value of this variable.
    /// @return the value of the variable
    const ValueType& getValue() const
    {
        return m_value;
    }

    /// @brief Set the value of this variable.
    /// @param value the value
    void setValue(const ValueType& value)
    {
        if (m_value != value)
        {
            m_value = value;
            ValueChanged();
        }
    }

    /// @brief Get the default value of the variable.
    /// @return the default value of this variable
    const ValueType& getDefaultValue() const
    {
        return m_defaultValue;
    }

    /// @brief Get the qualified name of this variable.
    /// @return the qualified name of this variable
    const std::string getName() const
    {
        return m_name;
    }

    /// @brief Get the description of this variable.
    /// @return the description of this variable
    const std::string getDescription() const
    {
        return m_description;
    }

    /// @brief Encode and store this element's value into to a string.
    /// @param target the target string
    /// @return @a true on success, @a false on failure
    virtual bool encodeValue(std::string& target) const = 0;

    /// @brief Load and decode this element's value from a string.
    /// @param source the source string
    /// @return @a true on success, @a false on failure
    virtual bool decodeValue(const std::string& source) = 0;

    /// @brief Event raised if the value of this variable has changed.
    Id::Signal<void()> ValueChanged;
};

/// @brief The abstract base of all variables.
/// @remark A variable consists of a name, a description, a default value, and a value.
template <typename ValueTypeArg, typename EnabledArg = void>
class Variable;

} // namespace Configuration
} // namespace Ego
