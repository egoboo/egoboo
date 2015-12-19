#pragma once

#include "egolib/platform.h"
#include "egolib/Math/_Include.hpp"

// Forward declaration.
class Object;

namespace Ego {
namespace Script {
namespace Interpreter {

/**
 * @brief An enumeration of tags identifiying types.
 */
enum class Tag {
    /// @brief The tag identifying the \f$Boolean\f$ type.
    Boolean,
    /// @brief The tag identifying the \f$Integer\f$ type.
    Integer,
    /// @brief The tag identifying the \f$Object\f$ type.
    Object,
    /// @brief The tag identifying the \f$Real\f$ type.
    Real,
    /// @brief The tag identifying the \f$Vector2\f$ type.
    Vector2,
    /// @brief The tag identifying the \f$Vector3\f$ type.
    Vector3,
    /// @brief The tag identifying the \f$Void\f$ type.
    Void,
}; // enum class Tag

inline std::string toString(Tag tag) {
    switch (tag) {
        case Tag::Boolean:
            return "Boolean";
        case Tag::Integer:
            return "Integer";
        case Tag::Object:
            return "Object";
        case Tag::Real:
            return "Real";
        case Tag::Vector2:
            return "Vector2";
        case Tag::Vector3:
            return "Vector3";
        case Tag::Void:
            return "Void";
        default:
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
}

/**
 * @brief An exception indicating an invalid cast.
 */
class InvalidCast : std::runtime_error {
    /**
     * @brief The tag of the source type.
     */
    Tag sourceTypeTag;
    /**
     * @brief The tag of the target type.
     */
    Tag targetTypeTag;

public:
    /**
     * @brief Construct this invalud cast exception.
     * @param sourceTypeTag the tag of the source type
     * @param targetTypeTag the tag of the target type
     */
    InvalidCast(Tag sourceTypeTag, Tag targetTypeTag)
        : std::runtime_error("invalid cast of a value of type " + toString(sourceTypeTag) + " into a value of type " + toString(targetTypeTag)),
        sourceTypeTag(sourceTypeTag), targetTypeTag(targetTypeTag) {}

    /**
     * @brief Get the tag of the source type.
     * @return the tag of the source type
     */
    Tag getSourceTypeTag() const {
        return sourceTypeTag;
    }

    /**
     * @brief Get the tag of the target type.
     * @return the tag of the target type
     */
    Tag getTargetTypeTag() const {
        return targetTypeTag;
    }

};

/**
 * @brief The C++ representation of a raw \f$Boolean\f$ value.
 */
typedef bool BooleanValue;

/**
 * @brief The C++ representation of a raw \f$Integer\f$ value.
 */
typedef int IntegerValue;

/**
 * @brief The C++ representation of a raw \f$Real\f$ value.
 */
typedef float RealValue;

/**
 * @brief The C++ representation of a raw \f$Object\f$ value.
 */
typedef std::shared_ptr<Object> ObjectValue;

/**
 * @brief The C++ representation of a raw \f$Vector2\f$ value.
 */
typedef Vector2f Vector2Value;

/**
 * @brief The C++ representation of a raw \f$Vector3\f$ value.
 */
typedef Vector3f Vector3Value;

/**
 * @brief The C++ representation of a raw \f$Void\f$ value.
 */
typedef struct VoidValue {} VoidValue;


/**
 * @brief Enable implicit conversions of numeric types such that \f$Integer\f$ and \f$Real\f$ can be converted into each other.
 */
#define Ego_Script_Interpreter_WithImplicitConversion (1)

 /**
  * @brief A tagged union of all values used by the interpreter \f$Tag \times Value\f$.
  */
struct TaggedValue {
    Tag tag; ///< @brief The tag denothing what kind of value is stored in this tagged value.
    union {
        BooleanValue booleanValue; ///< @brief A \f$Boolean\f$ value. Only valid if @a tag is Tag::Boolean.
        IntegerValue integerValue; ///< @brief A \f$Integer\f$ value. Only valid if @a tag is Tag::Integer.
        RealValue realValue; ///< @brief A \f$Real\f$ value. Only valid if @a tag is Tag::Real.
        ObjectValue objectValue; ///< @brief A \f$Object\f$ value. Only valid if @a tag is Tag::Object.
        Vector2Value vector2Value; ///< @brief A \f$Vector2\f$ value. Only valid if @a tag is Tag::Vector2.
        Vector3Value vector3Value; ///< @brief A \f$Vector3\f$ value. Only valid if @a tag is Tag::Vector3.
        VoidValue voidValue; ///< @brief A \f$Void\f$ value. Only valid if @a tag is Tag::Void.
    };

public:

    /**
     * @brief Copy-construct this tagged value with another tagged value.
     * @param other the other tagged value
     */
    TaggedValue(const TaggedValue& other) : tag(other.tag) {
        switch (tag) {
            case Tag::Boolean: booleanValue = other.booleanValue; break;
            case Tag::Integer: integerValue = other.integerValue; break;
            case Tag::Object: objectValue = other.objectValue; break;
            case Tag::Real: realValue = other.realValue; break;
            case Tag::Vector2: vector2Value = other.vector2Value; break;
            case Tag::Vector3: vector3Value = other.vector3Value; break;
            case Tag::Void: voidValue = other.voidValue; break;
            default: {
                tag = Tag::Void;
                voidValue = VoidValue();
                throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
            }
        };
    }

public:

    /**
     * @brief Construct this tagged value with the specified \f$Boolean\f$ value.
     * @param other the \f$Boolean\f$ value
     */
    TaggedValue(BooleanValue other) : tag(Tag::Boolean), booleanValue(other) {}

    /**
     * @brief Construct this tagged value with the specified \f$Integer\f$ value.
     * @param other the \f$Integer\f$ value
     */
    TaggedValue(IntegerValue other) : tag(Tag::Integer), integerValue(other) {}

    /**
     * @brief Construct this tagged value with the specified \Object\f$ value.
     * @param other the \Object\f$ value
     */
    TaggedValue(ObjectValue other) : tag(Tag::Object), objectValue(other) {}

    /**
     * @brief Construct this tagged value with the specified \f$Real\f$ value.
     * @param other the \f$Real\f$ value
     */
    TaggedValue(RealValue other) : tag(Tag::Real), realValue(other) {}

    /**
     * @brief Construct this tagged value with the specified \Vector2\f$ value.
     * @param other the \f$Vector2\f$ value
     */
    TaggedValue(Vector2Value other) : tag(Tag::Vector2), vector2Value(other) {}

    /**
     * @brief Construct this tagged value with the specified \f$Vector3\f$ value.
     * @param other the \f$Vector3\f$ value
     */
    TaggedValue(Vector3Value other) : tag(Tag::Vector3), vector3Value(other) {}

    /**
     * @brief Construct this tagged value with the specified \Void\f$ value.
     * @param other the \Void\f$ value
     */
    TaggedValue(VoidValue other) : tag(Tag::Void), voidValue(other) {}

public:
    operator BooleanValue() const {
        if (Tag::Boolean != tag) {
            throw InvalidCast(tag, Tag::Boolean);
        }
        return booleanValue;
    }

    operator IntegerValue() const {
#if defined(Ego_Script_Interpreter_WithImplicitConversion) && 1 == Ego_Script_Interpreter_WithImplicitConversion
        if (Tag::Integer == tag) return integerValue;
        else if (Tag::Real == tag) return realValue;
        else throw InvalidCast(tag, Tag::Real);
#else
        if (Tag::Integer != tag) {
            throw InvalidCast(tag, Tag::Integer);
        }
        return integerValue;
#endif
    }

    operator ObjectValue() const {
        if (Tag::Object != tag) {
            throw InvalidCast(tag, Tag::Object);
        }
        return objectValue;
    }

    operator RealValue() const {
#if defined(Ego_Script_Interpreter_WithImplicitConversion) && 1 == Ego_Script_Interpreter_WithImplicitConversion
        if (Tag::Integer == tag) return integerValue;
        else if (Tag::Real == tag) return realValue;
        else throw InvalidCast(tag, Tag::Real);
#else
        if (Tag::Real != tag) {
            throw InvalidCast(tag, Tag::Real);
        }
        return realValue;
#endif
    }

    operator Vector2Value() const {
        if (Tag::Vector2 != tag) {
            throw InvalidCast(tag, Tag::Vector2);
        }
        return vector2Value;
    }

    operator Vector3Value() const {
        if (Tag::Vector3 != tag) {
            throw InvalidCast(tag, Tag::Vector3);
        }
        return vector3Value;
    }

    operator VoidValue() const {
        if (Tag::Void != tag) {
            throw InvalidCast(tag, Tag::Void);
        }
        return voidValue;
    }

private:
    void destructValue() {
        switch (tag) {
            case Tag::Boolean:
            case Tag::Integer:
            case Tag::Real:
                break;
            case Tag::Object:
                objectValue.~ObjectValue();
                break;
            case Tag::Vector2:
                vector2Value.~Vector2Value();
                break;
            case Tag::Vector3:
                vector3Value.~Vector3Value();
                break;
            case Tag::Void:
                break;
        };
    }

public:
    /**
     * @brief Assign this tagged value with another tagged value.
     * @param other the other tagged value
     * @todo Use the swap idiom.
     */
    TaggedValue& operator=(const TaggedValue& other) {
        switch (tag) {
            case Tag::Boolean: booleanValue = other.booleanValue; break;
            case Tag::Integer: integerValue = other.integerValue; break;
            case Tag::Object: objectValue = other.objectValue; break;
            case Tag::Real: realValue = other.realValue; break;
            case Tag::Vector2: vector2Value = other.vector2Value; break;
            case Tag::Vector3: vector3Value = other.vector3Value; break;
            case Tag::Void: voidValue = other.voidValue; break;
            default:
            {
                tag = Tag::Void;
                voidValue = VoidValue();
                throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
            }
        };
        tag = other.tag;
        return *this;
    }

public:
    /**
     * @brief Assign this tagged value with the specified \f$Boolean\f$ value.
     * @param other the \f$Boolean\f$ value
     */
    TaggedValue& operator=(BooleanValue other) {
        destructValue();
        tag = Tag::Boolean;
        booleanValue = other;
        return *this;
    }

    /**
     * @brief Assign this tagged value with the specified \f$Integer\f$ value.
     * @param other the \f$Integer\f$ value
     */
    TaggedValue& operator=(IntegerValue other) {
        destructValue();
        tag = Tag::Integer;
        integerValue = other;
        return *this;
    }

    /**
     * @brief Assign this tagged value with the specified \f$Real\f$ value.
     * @param other the \f$Real\f$ value
     */
    TaggedValue& operator=(RealValue other) {
        destructValue();
        tag = Tag::Real;
        realValue = other;
        return *this;
    }

    /**
     * @brief Assign this tagged value with the specified \f$Vector2\f$ value.
     * @param other the \f$Vector2\f$ value
     */
    TaggedValue& operator=(Vector2Value other) {
        destructValue();
        tag = Tag::Vector2;
        vector2Value = other;
        return *this;
    }

    /**
     * @brief Assign this tagged value with the specified \f$Vector3\f$ value.
     * @param other the \f$Vector3\f$ value
     */
    TaggedValue& operator=(Vector3Value other) {
        destructValue();
        tag = Tag::Vector3;
        vector3Value = other;
        return *this;
    }

    /**
     * @brief Assign this tagged value with the specified \f$Void\f$ value.
     * @param other the \f$Void\f$ value
     */
    TaggedValue& operator=(VoidValue other) {
        destructValue();
        tag = Tag::Void;
        voidValue = other;
        return *this;
    }


public:
    ~TaggedValue() {
        destructValue();
    }

}; // struct TaggedValue
} // namespace Interpreter
} // namespace Script
} // namespace Ego