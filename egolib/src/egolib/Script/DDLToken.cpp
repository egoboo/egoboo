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
    : Id::Token<DDLTokenKind>(kind, startLocation, lexeme)
{}

DDLToken::DDLToken(const DDLToken& other)
    : Id::Token<DDLTokenKind>(other)
{}

DDLToken::DDLToken(DDLToken&& other)
    : Id::Token<DDLTokenKind>(std::move(other))
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
    os << "  " << "kind = " << token.getKind() << "," << std::endl;
    os << "  " << "lexeme = " << token.getLexeme() << "," << std::endl;
    os << "  " << "start location = " << token.getStartLocation().file_name() << ":" << token.getStartLocation().line_number() << "," << std::endl;
    os << "}" << std::endl;
    return os;
}

} // namespace Script
} // namespace Ego
