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

DDLToken::DDLToken(DDLToken::Kind kind, const Id::Location& startLocation, const std::string& lexeme)
    : m_kind(kind), m_startLocation(startLocation), m_lexeme(lexeme)
{}

DDLToken::DDLToken(const DDLToken& other)
    : m_kind(other.m_kind), m_startLocation(other.m_startLocation), m_lexeme(other.m_lexeme)
{}

DDLToken& DDLToken::operator=(const DDLToken& other)
{
    m_kind = other.m_kind;
    m_startLocation = other.m_startLocation;
    m_lexeme = other.m_lexeme;
    return *this;
}

DDLToken::Kind DDLToken::getKind() const
{
    return m_kind;
}

const std::string& DDLToken::getLexeme() const
{
    return m_lexeme;
}

const Id::Location& DDLToken::getStartLocation() const
{
    return m_startLocation;
}

} // namespace Script
} // namespace Ego
