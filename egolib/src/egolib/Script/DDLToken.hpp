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

/// @file egolib/Script/DDLToken.hpp
/// @brief Token of the DDL (Data Definition Language) of EgoScript.
/// @author Michael Heilmann

#pragma once

#include "egolib/Script/DDLTokenKind.hpp"

namespace Ego {
namespace Script {

/// @brief A token of the DDL (Data Definition Language) of EgoScript.
/// @todo Default token kind should be DDLTokenKind::Error.
struct DDLToken : public id::c::token<DDLTokenKind, DDLTokenKind::Unknown>
{
public:
    /// @brief Construct this token with the specified values.
    /// @param kind the kind of this token
    /// @param startLocation the start location of this token
    /// @param lexeme the lexeme of this token. Default is the empty string.
    DDLToken(DDLTokenKind kind, const id::c::location& startLocation, const std::string& lexeme = std::string());

    /// @brief Copy-construct this token from another token.
    /// @param other the other token
    DDLToken(const DDLToken& other);

    /// @brief Move-construct this token from another token.
    /// @param other the other token
    DDLToken(DDLToken&& other);

    /// @brief Assign this token from another token.
    /// @param other the other token
    /// @return this token
    DDLToken& operator=(DDLToken other);

    friend void swap(DDLToken& x, DDLToken& y)
    {
        using std::swap;
        swap(static_cast<id::c::token<DDLTokenKind, DDLTokenKind::Unknown>&>(x),
             static_cast<id::c::token<DDLTokenKind, DDLTokenKind::Unknown>&>(y));
    }

    /// @brief Overloaded &lt;&lt; operator for a token.
    /// @param ostream the output stream to write to
    /// @param token the token to write
    /// @return the output stream
    friend std::ostream& operator<<(std::ostream& os, const DDLToken& token);
};

std::ostream& operator<<(std::ostream& os, const DDLToken& token);

} // namespace Script
} // namespace Ego
