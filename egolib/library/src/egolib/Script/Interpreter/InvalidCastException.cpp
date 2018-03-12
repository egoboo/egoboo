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

/// @file   egolib/Script/Interpreter/InvalidCastException.hpp
/// @brief  An invalid cast exception.
/// @author Michael Heilmann

#include "egolib/Script/Interpreter/InvalidCastException.hpp"

#include "egolib/Script/Interpreter/Tag.hpp"

namespace Ego::Script::Interpreter {

InvalidCastException::InvalidCastException(Tag sourceTypeTag, Tag targetTypeTag)
    : std::runtime_error("invalid cast of a value of type " + toString(sourceTypeTag) + " into a value of type " + toString(targetTypeTag)),
    sourceTypeTag(sourceTypeTag), targetTypeTag(targetTypeTag) {}

Tag InvalidCastException::getSourceTypeTag() const {
    return sourceTypeTag;
}

Tag InvalidCastException::getTargetTypeTag() const {
    return targetTypeTag;
}

} // namespace Ego::Script::Interpreter
