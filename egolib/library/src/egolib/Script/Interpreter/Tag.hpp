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

/// @file   egolib/Script/Interpreter/Tag.hpp
/// @brief  A tag.
/// @author Michael Heilmann

#pragma once

#include "egolib/Script/Interpreter/Configuration.hpp"
#include <string>


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
#if defined(Ego_Script_Interpreter_WithProfileRefs) && 1 == Ego_Script_Interpreter_WithProfileRefs
	/// @brief The tag identifying the \$EnchantProfileRef\f$ type.
	EnchantProfileRef,
	/// @brief The tag identifying the \f$ParticleProfileRef\f$ type.
	ParticleProfileRef,
	/// @brief The tag identifying the \f$ObjectProfileRef\f$ type.
	ObjectProfileRef,
#endif
}; // enum class Tag

/**
 * @brief Get the type name denoted by a tag.
 * @param tag the tag
 * @return the type name denoted by the tag. The following mapping is applied:
 * - Tag::Boolean \f$\mapsto\f$ <c>Boolean</c>
 * - Tag::Integer \f$\mapsto\f$ <c>Integer</c>
 * - Tag::Object \f$\mapsto\f$ <c>Object</c>
 * - Tag::Real \f$\mapsto\f$ <c>Real</c>
 * - Tag::Vector2 \f$\mapsto\f$ <c>Vector2</c>
 * - Tag::Vector3 \f$\mapsto\f$ <c>Vector3</c>
 * - Tag::Void \f$\mapsto\f$ <c>Void</c>
 */
#if defined(Ego_Script_WithProfileRefs) && 1 == Ego_Script_WithProfileRefs
/**
 * - Tag::EnchantProfileRef \f$\mapsto\f$ <c>EnchantProfileRef</c>
 * - Tag::ParticleProfileRef \f$\mapsto\f$ <c>ParticleProfileRef</c>
 * - Tag::ObjectProfileRef \f$\mapsto\f$ <c>ObjectProfileRef</c>
 */
#endif
std::string toString(Tag tag);

} // namespace Interpreter
} // namespace Script
} // namespace Ego
