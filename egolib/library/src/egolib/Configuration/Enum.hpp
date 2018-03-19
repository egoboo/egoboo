#pragma once

#include "egolib/Configuration/Variable.hpp"
#include "egolib/Script/EnumDescriptor.hpp"

namespace Ego::Configuration {

/// @brief A variable storing an enum class value.
template <typename ValueType>
class Variable<ValueType, std::enable_if_t<std::is_enum<ValueType>::value>>
    : public VariableBase<ValueType>
{

private:

    // Static checking template arguments.
    static_assert(std::is_enum<ValueType>::value, "value type must be an enum type");

    /// @brief The descriptor of the enumeration.
    Ego::Script::EnumDescriptor<ValueType> enumDescriptor;

public:

    /// @brief Construct this variable.
    /// @param name the partially qualified name of the variable
    /// @param description the description of the variable
    Variable(const ValueType& defaultValue, const std::string& name, const std::string& description, const std::initializer_list<std::pair<const std::string, ValueType>> list) :
        VariableBase<ValueType>(defaultValue, name, description), enumDescriptor(name, list) {}

    Variable& operator=(const Variable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(std::string& target) const override
    {
        auto it = enumDescriptor.find(this->getValue());
        if (it == enumDescriptor.end())
        {
            return false;
        }
        target = it->first;
        return true;
    }

    virtual bool decodeValue(const std::string& source) override
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

} // namespace Ego::Configuration
