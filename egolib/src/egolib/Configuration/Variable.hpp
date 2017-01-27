#pragma once

#include "egolib/Script/EnumDescriptor.hpp"
#include "egolib/Script/Conversion.hpp"

namespace Ego {
namespace Configuration {

using namespace std;

/// @brief The base of all variables.
template <typename ValueTypeArg>
class VariableBase : public Id::NonCopyable
{
public:
    using ValueType = ValueTypeArg;

private:
    /// @brief The default value of the variable.
    const ValueType defaultValue;

    /// @brief The partially qualified name of this element e.g. <tt>video.fullscreen</tt>.
    string name;

    /// @brief The description of the variable e.g. <tt>Enable/disable fullscreen mode.</tt>.
    string description;

    /// @brief The value of the variable.
    ValueType value;

protected:
    /// @brief Construct this variable.
    /// @param defaultValue the default value
    /// @param name the qualified name of the variable
    /// @param description the description of the variable
    VariableBase(const ValueType& defaultValue, const string& name, const string& description) :
        defaultValue(defaultValue), name(name), description(description), value(defaultValue)
    {}

    /// @brief Destruct this variable.
    virtual ~VariableBase() {}

public:

    /// @brief Get the value of this variable.
    /// @return the value of the variable
    const ValueType& getValue() const
    {
        return value;
    }

    /// @brief Set the value of this variable.
    /// @param value the value
    void setValue(const ValueType& value)
    {
        this->value = value;
    }

    /// @brief Get the default value of the variable.
    /// @return the default value of this variable
    const ValueType& getDefaultValue() const
    {
        return defaultValue;
    }

    /// @brief Get the qualified name of this variable.
    /// @return the qualified name of this variable
    const string getName() const
    {
        return name;
    }

    /// @brief Get the description of this variable.
    /// @return the description of this variable
    const string getDescription() const
    {
        return description;
    }

    /// @brief Encode and store this element's value into to a string.
    /// @param target the target string
    /// @return @a true on success, @a false on failure
    virtual bool encodeValue(string& target) const = 0;

    /// @brief Load and decode this element's value from a string.
    /// @param source the source string
    /// @return @a true on success, @a false on failure
    virtual bool decodeValue(const string& source) = 0;
};

/// @brief The abstract base of all variables.
/// @remark A variable consists of a name, a description, a default value, and a value.
template <typename ValueTypeArg, typename EnabledArg = void>
class Variable;

} // namespace Configuration
} // namespace Ego

namespace std {

/// @brief
/// @code
/// is_any_of<T,A0,A1,..., An>::value
/// @endcode
/// is equivalent to
/// @code
/// is_same<T,A0>::value || is_same<T,A1>::value || ... | is_same<T,An>
/// @endcode
template<typename T, typename U, typename... Us>
struct is_any_of
    : integral_constant<
    bool,
    conditional<
    is_same<T, U>::value,
    true_type,
    is_any_of<T, Us...>
    >::type::value
    >
{};

template<typename T, typename U>
struct is_any_of<T, U> : is_same<T, U>::type {};

} // namespace std
