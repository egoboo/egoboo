#pragma once

#include "egolib/Configuration/Variable.hpp"
#include "egolib/Script/EnumDescriptor.hpp"

namespace Ego {
namespace Configuration {

/// @brief A variable storing an enum class value.
template <typename ValueType>
class Variable<ValueType, enable_if_t<is_enum<ValueType>::value>>
    : public VariableBase<ValueType>
{

private:

    // Static checking template arguments.
    static_assert(is_enum<ValueType>::value, "value type must be an enumeration");

    /// @brief The descriptor of the enumeration.
    Ego::Script::EnumDescriptor<ValueType> enumDescriptor;

public:

    /// @brief Construct this variable.
    /// @param name the partially qualified name of the variable
    /// @param description the description of the variable
    Variable(const ValueType& defaultValue, const string& name, const string& description, const initializer_list<pair<const string, ValueType>> list) :
        VariableBase<ValueType>(defaultValue, name, description), enumDescriptor(name, list) {}

    Variable& operator=(const Variable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override
    {
        auto it = enumDescriptor.find(this->getValue());
        if (it == enumDescriptor.end())
        {
            return false;
        }
        target = it->first;
        return true;
    }

    virtual bool decodeValue(const string& source) override
    {
        auto it = enumDescriptor.find(source);
        if (it == enumDescriptor.end())
        {
            return false;
        }
        this->setValue(it->second);
        return true;
    }
};

} // namespace Configuration
} // namespace Ego
