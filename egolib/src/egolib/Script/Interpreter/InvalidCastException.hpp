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

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Script {
namespace Interpreter {

// Forward declaration.
enum class Tag;

/**
 * @brief An exception indicating an invalid cast.
 */
class InvalidCastException : public std::runtime_error {
private:
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
     * @brief Construct this invalid cast exception.
     * @param sourceTypeTag the tag of the source type
     * @param targetTypeTag the tag of the target type
     */
    InvalidCastException(Tag sourceTypeTag, Tag targetTypeTag);

    /**
     * @brief Get the tag of the source type.
     * @return the tag of the source type
     */
    Tag getSourceTypeTag() const;

    /**
     * @brief Get the tag of the target type.
     * @return the tag of the target type
     */
    Tag getTargetTypeTag() const;

}; // class InvalidCastException

} // namespace Interpreter
} // namespace Script
} // namespace Ego
