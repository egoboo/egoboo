#pragma once

#include "egolib/Configuration/Variable.hpp"
#include "idlib/hll.hpp"

namespace Ego {
namespace Configuration {

/// @brief A variable storing a numeric value.
template <class ValueType>
class Variable<ValueType, std::enable_if_t<!std::is_same<ValueType, bool>::value && std::is_arithmetic<ValueType>::value>>
    : public VariableBase<ValueType>
{
    // (u)intx_t, x in [8,16,32,64] & float & double
    static_assert(!std::is_same<ValueType, bool>::value && std::is_arithmetic<ValueType>::value, "ValueType must not be an arithmetic non-bool type");

private:
    /// @brief The minimum value (inclusive).
    /// @invariant  <tt>min &lt= max</tt>
    ValueType min;

    /// @brief The maximum value (inclusive).
    /// @invariant <tt>min &lt= max</tt>
    ValueType max;

public:
    /// @brief Construct this variable.
    /// @param defaultValue the default value the variable
    /// @param name the partially qualified name of the variable
    /// @param description the description of the variable
    /// @param min, max the minimum (incl.) and the maximum value (incl.)
    /// @throw std::invalid_argument
    /// <tt>min > max</tt> or <tt>defaultValue < min</tt> or <tt>defaultValue > max</tt>
    Variable(ValueType defaultValue, const std::string& name, const std::string& description,
             ValueType min = std::numeric_limits<ValueType>::min(), 
             ValueType max = std::numeric_limits<ValueType>::max()) :
        VariableBase<ValueType>(defaultValue, name, description), min(min), max(max)
    {
        if (min > max) throw std::invalid_argument("min > max");
        else if (defaultValue < min) throw std::invalid_argument("defaultValue < min");
        else if (defaultValue > max) throw std::invalid_argument("defaultValue > max");
    }

    const Variable& operator=(const Variable& other)
    {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(std::string& target) const override
    {
        return idlib::hll::encoder<ValueType>()(this->getValue(), target);
    }

    virtual bool decodeValue(const std::string& source) override
    {
        ValueType temporary = {};
        if (!idlib::hll::decoder<ValueType>()(source, temporary))
        {
            return false;
        }
        this->setValue(temporary);
        return true;
    }

    /// @brief Get the maximum value.
    /// @return the maximum value
    ValueType getMaxValue() const { return max; }

    /// @brief Get the minimum value.
    /// @return the minimum value
    ValueType getMinValue() const { return min; }
};

} // namespace Configuration
} // namespace Ego
