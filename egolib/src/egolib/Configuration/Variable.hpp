#pragma once

#include "egolib/typedef.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/Script/EnumDescriptor.hpp"
#include "egolib/FileFormats/configfile.h"
#include "egolib/Script/Conversion.hpp"
#include "egolib/Renderer/TextureFilter.hpp"

namespace Ego {
namespace Configuration {

using namespace std;

template <typename ValueType>
class Variable : public Id::NonCopyable {

private:

    /**
     * @brief
     *  The default value of the variable.
     */
    const ValueType _defaultValue;

    /**
     * @brief
     *  The partially qualified name of this element e.g. <tt>video.fullscreen</tt>.
     */
    string _name;

    /**
     * @brief
     *  The description of the variable e.g. <tt>Enable/disable fullscreen mode.</tt>.
     */
    string _description;

    /**
     * @brief
     *  The value of the variable.
     */
    ValueType _value;

protected:

    /**
     * @brief
     * @param defaultValue
     *  the default value
     * @param qualifiedName
     *  the qualified name of the variable
     * @param description
     *  the description of the variable
     */
    Variable(const ValueType& defaultValue, const string& name, const string& description) :
        _defaultValue(defaultValue), _name(name), _description(description),
        _value(defaultValue) {}

    /**
     * @brief
     *  Destruct this variable.
     */
    virtual ~Variable() {}

public:

    /**
     * @brief
     *  Get the value of this variable.
     * @return
     *  the value of the variable
     */
    const ValueType& getValue() const {
        return _value;
    }

    /**
     * @brief
     *  Set the value of this variable.
     * @param value
     *  the value
     */
    void setValue(const ValueType& value) {
        _value = value;
    }

    /**
     * @brief
     *  Get the default value of the variable.
     * @return
     *  the default value of this variable
     */
    const ValueType& getDefaultValue() const {
        return _defaultValue;
    }

    /**
     * @brief
     *  Get the qualified name of this variable.
     * @return
     *  the qualified name of this variable
     */
    const string getName() const {
        return _name;
    }

    /**
     * @brief
     *  Get the description of this variable.
     * @return
     *  the description of this variable
     */
    const string getDescription() const {
        return _description;
    }

    /**
     * @brief
     *  Encode and store this element's value into to a string.
     * @param target
     *  the target string
     * @return
     *  @a true on success, @a false on failure
     */
    virtual bool encodeValue(string& target) const = 0;

    /**
     * @brief
     *  Load and decode this element's value from a string.
     * @param source
     *  the source string
     * @return
     *  @a true on success, @a false on failure
     */
    virtual bool decodeValue(const string& source) = 0;
};

} // namespace Configuration
} // namespace Ego

