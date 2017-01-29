#pragma once

#include "egolib/Configuration/Variable.hpp"

namespace Ego {
namespace Configuration {

/// @brief A variable storing either a @a bool or and @a std::string value.
template <class ValueType>
class Variable<ValueType, enable_if_t<is_any_of<ValueType, string, bool>::value>>
    : public VariableBase<ValueType>
{
    static_assert(is_any_of<ValueType, string, bool>::value, "ValueType must not be an enumeration type");

public:

    /// @brief Construct this standard variable.
    /// @param defaultValue the default value
    /// @param name the qualified name of the variable
    /// @param description the description of the variable
    Variable(const ValueType& defaultValue, const string& name, const string& description) :
        VariableBase<ValueType>(defaultValue, name, description) {}

    Variable& operator=(const Variable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override
    {
        return Script::Encoder<ValueType>()(this->getValue(), target);
    }

    virtual bool decodeValue(const string& source) override
    {
        ValueType temporary;
        if (!Script::Decoder<ValueType>()(source, temporary))
        {
            return false;
        }
        this->setValue(temporary);
        return true;
    }

};

} // namespace Configuration
} // namespace Ego
