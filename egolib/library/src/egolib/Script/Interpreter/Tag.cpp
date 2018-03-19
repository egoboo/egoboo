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

/// @file   egolib/Script/Interpreter/Tag.cpp
/// @brief  A tag.
/// @author Michael Heilmann

#include "egolib/Script/Interpreter/Tag.hpp"
#include "idlib/idlib.hpp"

namespace Ego::Script::Interpreter {

std::string toString(Tag tag) {
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
#if defined(Ego_Script_WithProfileRefs) && 1 == Ego_Script_WithProfileRefs
		case Tag::EnchantProfileRef:
			return "EnchantProfileRef";
		case Tag::ParticleProfileRef:
			return "ParticleProfileRef";
		case Tag::ObjectProfileRef:
			return "ObjectProfileRef";
#endif
        default:
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    }
}

} // namespace Ego::Script::Interpreter
