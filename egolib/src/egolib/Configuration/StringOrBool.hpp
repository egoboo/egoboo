#pragma once

#include "egolib/Configuration/Variable.hpp"
#include "idlib/is_any_of.hpp"

namespace Ego {
namespace Configuration {

/// @brief A variable storing either a @a bool or and @a std::string value.
template <class ValueType>
class Variable<ValueType, std::enable_if_t<id::is_any_of<ValueType, std::string, bool>::value>>
    : public VariableBase<ValueType>
{
    static_assert(id::is_any_of<ValueType, std::string, bool>::value, "ValueType must be std::string or bool type");

public:

    /// @brief Construct this standard variable.
    /// @param defaultValue the default value
    /// @param name the qualified name of the variable
    /// @param description the description of the variable
    Variable(const ValueType& defaultValue, const std::string& name, const std::string& description) :
        VariableBase<ValueType>(defaultValue, name, description) {}

    Variable& operator=(const Variable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(std::string& target) const override
    {
        return Script::Encoder<ValueType>()(this->getValue(), target);
    }

    virtual bool decodeValue(const std::string& source) override
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
