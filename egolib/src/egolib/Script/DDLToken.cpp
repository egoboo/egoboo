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

/// @file egolib/Script/DDLToken.cpp
/// @brief Token of the DDL (Data Definition Language) of EgoScript.
/// @author Michael Heilmann

#include "egolib/Script/DDLToken.hpp"


namespace Ego {
namespace Script {

DDLToken::DDLToken(DDLTokenKind kind, const id::location& startLocation, const std::string& lexeme)
    : id::token<DDLTokenKind, DDLTokenKind::Unknown>(kind, startLocation, lexeme)
{}

DDLToken::DDLToken(const DDLToken& other)
    : id::token<DDLTokenKind, DDLTokenKind::Unknown>(other)
{}

DDLToken::DDLToken(DDLToken&& other)
    : id::token<DDLTokenKind, DDLTokenKind::Unknown>(std::move(other))
{}

DDLToken& DDLToken::operator=(DDLToken other)
{
    swap(*this, other);
    return *this;
}

std::ostream& operator<<(std::ostream& os, const DDLToken& token)
{
    os << "ddl token" << std::endl;
    os << "{" << std::endl;
    os << "  " << "category = " << token.category() << "," << std::endl;
    os << "  " << "lexeme = " << token.get_lexeme() << "," << std::endl;
    os << "  " << "start location = " << token.get_start_location().file_name() << ":" << token.get_start_location().line_number() << "," << std::endl;
    os << "}" << std::endl;
    return os;
}

} // namespace Script
} // namespace Ego
