#pragma once

#include "egolib/Configuration/NumericVariable.hpp"

namespace Ego {
namespace Configuration {

template <class ValueType>
class NumericVariable : public Variable<ValueType> {
    // (u)intx_t, x in [8,16,32,64] & float & double
    static_assert(!is_same<ValueType, bool>::value && is_arithmetic<ValueType>::value, "ValueType must not be an arithmetic non-bool type");

private:
    /**
     * @brief
     *  The minimum value (inclusive).
     * @invariant
     *   <tt>_min <= _max</tt>
     */
    ValueType _min;
    
    /**
     * @brief
     *  The maximum value (inclusive).
     * @invariant
     *  <tt>_min <= _max</tt>
     */
    ValueType _max;

public:
    /**
     * @brief
     * @param defaultValue
     *  the default value the variable
     * @param name
     *  the partially qualified name of the variable
     * @param description
     *  the description of the variable
     * @throw std::invalid_argument
     *  if <tt>min > max</tt> or <tt>defaultValue < min</tt> or <tt>defaultValue > max</tt>
     */
    NumericVariable(const ValueType& defaultValue, const string& name, const string& description,
                    const ValueType& min, const ValueType& max) :
        Variable<ValueType>(defaultValue, name, description), _min(min), _max(max) {
        if (min > max) throw std::invalid_argument("min > max");
        else if (defaultValue < min) throw std::invalid_argument("defaultValue < min");
        else if (defaultValue > max) throw std::invalid_argument("defaultValue > max");
    }

    const NumericVariable& operator=(const NumericVariable& other) {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override {
        return Ego::Script::Encoder<ValueType>()(this->getValue(), target);
    }

    virtual bool decodeValue(const string& source) override {
        ValueType temporary = {};
        if (!Ego::Script::Decoder<ValueType>()(source, temporary)) {
            return false;
        }
        this->setValue(temporary);
        return true;
    }

    ValueType getMaxValue() const { return _max; }

    ValueType getMinValue() const { return _min; }
};

} // namespace Configuration
} // namespace Ego
