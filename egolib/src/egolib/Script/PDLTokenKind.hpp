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

/// @file egolib/Script/PDLTokenKind.hpp
/// @brief Token kinds of the PDL (Program Definition Language) of EgoScript.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Script {

/// @brief An enumeration of the token kinds of the PDL of EgoScript.
enum class PDLTokenKind
{
    Unknown,
    Function,
    Variable,
    Constant,

    Assign, ///< Token type of an "assign" operator.
    And, ///< Token type of an "and" operator.
    Plus, ///< Token type of a "plus" ("add" or "unary plus") operator.
    Minus, ///< Token type of a "minus" ("subtract" or "unary minus") operator.
    Multiply, ///< Token type of a "multiply" operator.
    Divide, ///< Token type of a "divide" operator.
    Modulus, ///< Token type of a "modulus" operator.
    ShiftRight, ///< Token type of a "shift right" operator.
    ShiftLeft, ///< Token type of a "shift left" operator.

    /// Token type of zero or more whitespaces.
    /// getValue indicates the number of whitespaces.
    Whitespace,

    /// Token type of zero or more newlines.
    /// getValue indicates the number of newlines.
    Newline,

    /// Token type of zero to 15 indentions.
    /// getValue indicates the number of indentions.
    Indent,

    /// Token type of the end of a line.
    EndOfLine,

    /// Token type of a numeric literal.
    NumericLiteral,

    /// Token type of a name.
    Name,

    /// Token type of a string.
    String,

    /// Token type of a reference.
    Reference,

    /// Token type of an IDSZ.
    IDSZ,

}; // enum class PDLTokenKind

} // namespace Script
} // namespace Ego
