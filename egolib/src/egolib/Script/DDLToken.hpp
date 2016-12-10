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
struct DDLToken
{
private:
    /// @brief The kind of this token.
    DDLTokenKind m_kind;

    /// @brief The start location of this token.
    Id::Location m_startLocation;

    /// @brief The lexeme of this token.
    std::string m_lexeme;

public:
    /// @brief Construct this token.
    /// @param kind the kind of this token
    /// @param startLocation the start location of this token
    /// @param lexeme the lexeme of this token. Default is the empty string.
    DDLToken(DDLTokenKind kind, const Id::Location& startLocation, const std::string& lexeme = std::string());

    /// @brief Copy-construct this token from another token.
    /// @param other the other token
    DDLToken(const DDLToken& other);

    /// @brief Get the kind of this token.
    /// @return the kind of this token
    DDLTokenKind getKind() const;

    /// @brief Get the start location of this token.
    /// @return the start location of this token
    const Id::Location& getStartLocation() const;

    /// @brief Assign this token from another token.
    /// @param other the construction source
    /// @return this token
    DDLToken& operator=(const DDLToken& other);

    /// @brief Get the lexeme of this token.
    /// @return the lexeme of this token
    const std::string& getLexeme() const;

};

} // namespace Script
} // namespace Ego
