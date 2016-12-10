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

/// @file egolib/Script/DDLTokenKind.hpp
/// @brief Token kinds of the DDL (Data Definition Language) of EgoScript.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Script {

/// @brief An enumeration of the token kinds of the DDL of EgoScript.
enum class DDLTokenKind
{
    /// @brief An integer number literal.
    Integer,

    /// @brief A real number literal.
    Real,

    /// @brief A character literal.
    Character,

    /// @brief A string literal.
    String,

    /// @brief A boolean literal.
    Boolean,

    /// @brief A name (aka identifier).
    Name,

    /// @brief A comment.
    Comment,

}; // enum DDLTokenKind

} // namespace Script
} // namespace Ego
