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

/// @file   egolib/Script/Token.cpp
/// @brief  A basic token class
/// @author Michael Heilmann

#include "egolib/Script/Token.hpp"


namespace Ego {
namespace Script {

TextToken::TextToken(TextToken::Type type, const Id::Location& location, const std::string& lexeme)
	: _type(type), _location(location), _lexeme(lexeme)
{}

TextToken::TextToken(const TextToken& other)
	: _type(other._type), _location(other._location), _lexeme(other._lexeme)
{}

TextToken& TextToken::operator=(const TextToken& other) {
	_type = other._type;
	_location = other._location;
	_lexeme = other._lexeme;
	return *this;
}

TextToken::Type TextToken::getType() const {
	return _type;
}

const std::string& TextToken::getLexeme() const {
	return _lexeme;
}

const Id::Location& TextToken::getLocation() const {
	return _location;
}

} // namespace Script
} // namespace Ego
