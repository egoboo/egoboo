#pragma once

#include "egolib/Configuration/Variable.hpp"

namespace Ego {
namespace Configuration {

/**
 * @brief
 *  An element of a configuration with a "name" and a "desciption".
 * @remark
 *  We can't use a single variable template and use SFINAE to specialize for
 *  certain types (i.e. for enumerations, for integral types, etc.) because
 *  Redmond Retards don't support SFINAE yet - in fact MSVC crashes upon
 *  <tt>enable_if<is_enum<ValueType>::value>::type</tt>.
 *  See
 *  - http://en.cppreference.com/w/cpp/types/is_enum
 *  - http://en.cppreference.com/w/cpp/types/enable_if and
 *  - http://en.cppreference.com/w/cpp/language/sfinae
 *  for more information.
 */
template <typename ValueType>
class EnumerationVariable : public Variable<ValueType> {

private:

    // Static checking template arguments.
    static_assert(is_enum<ValueType>::value, "value type must be an enumeration");

    /**
     * @brief
     *  The descriptor of the enumeration.
     */
    Ego::Script::EnumDescriptor<ValueType> _enumDescriptor;

public:

    /**
     * @brief
     * @param name
     *  the partially qualified name of the variable
     * @param description
     *  the description of the variable
     */
    EnumerationVariable(const ValueType& defaultValue, const string& name, const string& description, const initializer_list<pair<const string, ValueType>> list) :
        Variable<ValueType>(defaultValue, name, description), _enumDescriptor(name, list) {}

    EnumerationVariable& operator=(const EnumerationVariable& other) {
        this->setValue(other.getValue());
        return *this;
    }

    virtual bool encodeValue(string& target) const override {
        auto it = _enumDescriptor.find(this->getValue());
        if (it == _enumDescriptor.end()) {
            return false;
        }
        target = it->first;
        return true;
    }

    virtual bool decodeValue(const string& source) override {
        auto it = _enumDescriptor.find(source);
        if (it == _enumDescriptor.end()) {
            return false;
        }
        this->setValue(it->second);
        return true;
    }
};

} // namespace Configuration
} // namespace Ego
