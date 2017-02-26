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

/// @file egolib/Script/PDLToken.cpp
/// @brief Token of the PDL (Program Definition Language) of EgoScript.
/// @author Michael Heilmann

#include "egolib/Script/PDLToken.hpp"

namespace Ego {
namespace Script {

PDLToken::PDLToken()
    : Id::Token<PDLTokenKind>(PDLTokenKind::Unknown, id::location("<unknown>", 1), std::string()), m_endLocation("<unknown>", 1), m_value(0)
{}

PDLToken::PDLToken(PDLTokenKind kind, const id::location& startLocation,
                   const id::location& endLocation, const std::string& lexeme)
    : Id::Token<PDLTokenKind>(kind, startLocation, lexeme), m_endLocation(endLocation), m_value(0)
{}

PDLToken::PDLToken(const PDLToken& other)
    : Id::Token<PDLTokenKind>(other), m_endLocation(other.m_endLocation), m_value(other.m_value)
{}

PDLToken::PDLToken(PDLToken&& other)
    : Id::Token<PDLTokenKind>(std::move(other)), m_endLocation(std::move(other.m_endLocation)), m_value(std::move(other.m_value))
{}

PDLToken::~PDLToken()
{}

bool PDLToken::isOperator() const
{
    return isOneOf(PDLTokenKind::Assign,
                   PDLTokenKind::Plus,
                   PDLTokenKind::Minus,
                   PDLTokenKind::And,
                   PDLTokenKind::Multiply,
                   PDLTokenKind::Divide,
                   PDLTokenKind::Modulus,
                   PDLTokenKind::ShiftRight,
                   PDLTokenKind::ShiftLeft);
}

bool PDLToken::isAssignOperator() const
{
    return is(PDLTokenKind::Assign);
}

bool PDLToken::isLiteral() const
{
    return isOneOf(PDLTokenKind::IdszLiteral,
                   PDLTokenKind::NumberLiteral,
                   PDLTokenKind::ReferenceLiteral,
                   PDLTokenKind::StringLiteral);
}

bool PDLToken::isConstant() const
{
    return is(PDLTokenKind::Constant);
}

PDLToken& PDLToken::operator=(PDLToken other)
{
    swap(*this, other);
    return *this;
}

std::ostream& operator<<(std::ostream& os, const PDLToken& token)
{
    os << "pdl token" << std::endl;
    os << "{" << std::endl;
    os << "  " << "kind = " << token.getKind() << "," << std::endl;
    os << "  " << "lexeme = " << token.getLexeme() << "," << std::endl;
    os << "  " << "start location = " << token.getStartLocation().file_name() << ":" << token.getStartLocation().line_number() << "," << std::endl;
    os << "  " << "end location = " << token.getEndLocation().file_name() << ":" << token.getEndLocation().line_number() << "," << std::endl;
    os << "  " << "value = " << token.getValue() << std::endl;
    os << "}" << std::endl;
    return os;
}

} // namespace Script
} // namespace Ego
