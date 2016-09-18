//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file   egolib/Script/Interpreter/TaggedValue.hpp
/// @brief  A tagged value.
/// @author Michael Heilmann

#pragma once

#include "egolib/Script/Interpreter/Configuration.hpp"
#include "egolib/Math/_Include.hpp"

// Forward declaration.
class Object;

namespace Ego {
namespace Script {
namespace Interpreter {

// Foward declaration.
enum class Tag;

/// @brief The C++ representation of a raw \f$Boolean\f$ value.
using BooleanValue = bool;

/// @brief The C++ representation of a raw \f$Object\f$ value.
using ObjectValue = std::shared_ptr<Object>;

/// @brief The C++ representation of a raw \f$Vector2\f$ value.
using Vector2Value = Vector2f;

/// @brief The C++ representation of a raw \f$Vector3\f$ value.
using Vector3Value = Vector3f;

/// @brief The C++ representation of a raw \f$Void\f$ value.
struct VoidValue {};

#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
/// @brief The C++ representation of a raw \f$EnchantProfileRefValue\f$.
using EnchantProfileRefValue = EnchantProfileRef;

/// @brief The C++ representation of a raw \f$ParticleProfileRefValue\f$.
using ParticleProfileRefValue = ParticleProfileRef;

/// @brief The C++ representation of a raw \f$ObjectProfileRefValue\f$.
using ObjectProfileRefValue = ObjectProfileRef;
#endif

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
    #if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
		ObjectProfileRefValue objectProfileRefValue; ///< @brief An \f$ObjectProfileRef\f$ value. Only valid if @a tag is Tag::ObjectProfileRef.
		ParticleProfileRefValue particleProfileRefValue; ///< @brief A \f$ParticleProfileRef\f$\f$ value. Only valid if @a tag is Tag::ParticleProfileRef.
		EnchantProfileRefValue enchantProfileRefValue; ///< @brief A \$EnchantProfileRef\f$ value. Only valid if @a tag is Tag::EnchantProfileRef.
    #endif
    };

public:

    /// @brief Default construct this tagged value.
    /// @post The \f$Void\f$ value is assigned to this tagged value.
    TaggedValue();

    /// @brief Copy-construct this tagged value with another tagged value.
    /// @param other the other tagged value
    TaggedValue(const TaggedValue& other);

public:

    /// @brief Construct this tagged value with the specified \f$Boolean\f$ value.
    /// @param other the \f$Boolean\f$ value
    TaggedValue(BooleanValue other);

    /// @brief Construct this tagged value with the specified \f$Integer\f$ value.
    /// @param other the \f$Integer\f$ value
    TaggedValue(IntegerValue other);

    /// @brief Construct this tagged value with the specified \f$Object\f$ value.
    /// @param other the \f$Object\f$ value
    TaggedValue(ObjectValue other);

    /// @brief Construct this tagged value with the specified \f$Real\f$ value.
    /// @param other the \f$Real\f$ value
    TaggedValue(RealValue other);

	/// @brief Construct this tagged value with the specified \f$Vector2\f$ value.
    /// @param other the \f$Vector2\f$ value
    TaggedValue(Vector2Value other);

    /// @brief Construct this tagged value with the specified \f$Vector3\f$ value.
    /// @param other the \f$Vector3\f$ value    
    TaggedValue(Vector3Value other);

    /// @brief Construct this tagged value with the specified \f$Void\f$ value.
    /// @param other the \f$Void\f$ value
    TaggedValue(VoidValue other);
	
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
	
    /// @brief Construct this tagged value with the specified \f$EnchantProfileRef\f$ value.
    /// @param other the \f$EnchantProfileRef\f$ value
    TaggedValue(EnchantProfileRefValue other);
	
    /// @brief Construct this tagged value with the specified \f$ParticleProfileRef\f$ value.
    /// @param other the \f$ParticleProfileRef\f$ value
    TaggedValue(ParticleProfileRefValue other);
	
    /// @brief Construct this tagged value with the specified \f$ObjectProfileRef\f$ value.
    /// @param other the \f$ObjectProfileRef\f$ value
    TaggedValue(ObjectProfileRefValue other);
	
#endif

public:
	/// Convert this tagged value into a \f$Boolean\f$ value.
    operator BooleanValue() const;

	/// Convert this tagged value into an \f$Integer\f$ value.
    operator IntegerValue() const;

	/// Convert this tagged value into an \f$Object\f$ value.
    operator ObjectValue() const;

	/// @brief Convert this tagged value into a \f$Real\f$ value.
    operator RealValue() const;

	/// @brief Convert this tagged value into a \f$Vector2\f$ value.
    operator Vector2Value() const;

	/// @brief Convert this tagged value into a \f$Vector3\f$ value.
    operator Vector3Value() const;

	/// @brief Convert this tagged value into a \f$Void\f$ value.
    operator VoidValue() const;
	
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs

	/// @brief Convert this tagged value into a \f$EnchantProfileRef\f$ value.
    operator EnchantProfileRefValue() const;

	/// @brief Convert this tagged value into a \f$ParticleProfileRef\f$ value.
    operator ParticleProfileRefValue() const;

	/// @brief Convert this tagged value into a \f$ObjectProfileRef\f$ value.
    operator ObjectProfileRefValue() const;

#endif

private:
    void destructValue();

public:
    /// @brief Assign this tagged value with another tagged value.
    /// @param other the other tagged value
    /// @todo Use the swap idiom.
    TaggedValue& operator=(const TaggedValue& other);

public:
    /// @brief Assign this tagged value with the specified \f$Boolean\f$ value.
    /// @param other the \f$Boolean\f$ value
    TaggedValue& operator=(BooleanValue other);

    /// @brief Assign this tagged value with the specified \f$Integer\f$ value.
    /// @param other the \f$Integer\f$ value
    TaggedValue& operator=(IntegerValue other);

    /// @brief Assign this tagged value with the specified \f$Real\f$ value.
    /// @param other the \f$Real\f$ value
    TaggedValue& operator=(RealValue other);

    /// @brief Assign this tagged value with the specified \f$Vector2\f$ value.
    /// @param other the \f$Vector2\f$ value
    TaggedValue& operator=(Vector2Value other);

    /// @brief Assign this tagged value with the specified \f$Vector3\f$ value.
    /// @param other the \f$Vector3\f$ value
    TaggedValue& operator=(Vector3Value other);

    /// @brief Assign this tagged value with the specified \f$Void\f$ value.
    /// @param other the \f$Void\f$ value
    TaggedValue& operator=(VoidValue other);
	
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs

    /// @brief Assign this tagged value with the specified \f$EnchantProfileRef\f$ value.
    /// @param other the \f$EnchantProfileRef\f$ value
    TaggedValue& operator=(EnchantProfileRefValue other);

    /// @brief Assign this tagged value with the specified \f$ParticleProfileRef\f$ value.
    /// @param other the \f$ParticleProfileRef\f$ value
    TaggedValue& operator=(ParticleProfileRefValue other);

    /// @brief Assign this tagged value with the specified \f$ObjectProfileRef\f$ value.
    /// @param other the \f$ObjectProfileRef\f$ value
    TaggedValue& operator=(ObjectProfileRefValue other);
	
#endif

public:
    ~TaggedValue();

public:
    /// @brief Get the tag of this tagged value.
    /// @return the tag of this tagged value
    Tag getTag() const;


}; // struct TaggedValue

} // namespace Interpreter
} // namespace Script
} // namespace Ego

/// @brief Overloaded &lt;&lt; operator for a tagged values.
/// @param ostream the output stream to write to
/// @param taggedValue the tagged value to write
std::ostream& operator<<(std::ostream& os, const Ego::Script::Interpreter::TaggedValue& taggedValue);
