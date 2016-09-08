#pragma once

#include "egolib/Configuration/Variable.hpp"

namespace Ego {
namespace Configuration {

/**
* @brief
*  An element of a configuration with a "name" and a "desciption".
* @remark
*  An extra specialization of @a ValueType is an enumeration type is provided.
*/
template <class ValueType>
class StandardVariable : public Variable<ValueType> {
    static_assert(!is_enum<ValueType>::value, "ValueType must not be an enumeration type");

public:

    /**
    * @brief
    * @param defaultValue
    *  the default value
    * @param name
    *  the qualified name of the variable
    * @param description
    *  the description of the variable
    */
    StandardVariable(const ValueType& defaultValue, const string& name, const string& description) :
        Variable<ValueType>(defaultValue, name, description) {}

    StandardVariable& operator=(const StandardVariable& other) {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override {
        return Ego::Script::Encoder<ValueType>()(this->getValue(), target);
    }

    virtual bool decodeValue(const string& source) override {
        ValueType temporary;
        if (!Ego::Script::Decoder<ValueType>()(source, temporary)) {
            return false;
        }
        this->setValue(temporary);
        return true;
    }

};

} // namespace Configuration
} // namespace Ego
