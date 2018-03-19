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
#include <string>

namespace Ego::Script {

/// @brief An enumeration of the token kinds of the DDL of EgoScript.
enum class DDLTokenKind
{
#define Define(name, string) name,
#include "egolib/Script/DDLTokenKind.in"
#undef Define
}; // enum DDLTokenKind

/// @brief Get a human-readable string describing a token kind.
/// @param kind the token kind
/// @return a human-readable string describing the token kind @a kind
std::string toString(DDLTokenKind kind);

/// @brief Overloaded &lt;&lt; operator for a token kind.
/// @param ostream the output stream to write to
/// @param tokenType the token kind to write
/// @return the output stream
std::ostream& operator<<(std::ostream& os, const DDLTokenKind& kind);

} // namespace Ego::Script
