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
#include "game/script_scanner.hpp"

Token::Token()
	: _text(), _location("<unknown>", 1), _value(0), _type(Type::Unknown) 
{}

Token::Token(Type type, const Location& location)
    : _text(), _location(location), _value(0), _type(type)
{}

Token::Token(const Token& other)
	: _text(other._text),  _location(other._location), _value(other._value), _type(other._type)
{}

Token::~Token()
{}

bool Token::is(Token::Type type) const
{
    return type == getType();
}

bool Token::isOneOf(Token::Type type0, Token::Type type1) const
{
    return is(type0) || is(type1);
}

bool Token::isOperator() const
{
    return isOneOf(Type::Assign,
                   Type::Plus,
                   Type::Minus,
                   Type::And,
                   Type::Multiply,
                   Type::Divide,
                   Type::Modulus,
                   Type::ShiftRight,
                   Type::ShiftLeft);
}

bool Token::isAssignOperator() const
{
    return is(Token::Type::Assign);
}

Location Token::getLocation() const
{
    return _location;
}

void Token::setLocation(const Location& location)
{
    _location = location;
}

Token::Type Token::getType() const
{
    return _type;
}

void Token::setType(Token::Type type)
{
    _type = type;
}

void Token::setText(const std::string& text)
{
    _text = text;
}

const std::string& Token::getText() const
{
    return _text;
}

std::ostream& operator<<(std::ostream& os, const Token::Type& tokenType) {
	switch (tokenType) {
    #define Define(enumElementName, string) case Token::Type::enumElementName: os << string; break;
		Define(Constant, "constant")
		Define(Function, "function")
        Define(Assign, "assign")
        Define(And, "and")
        Define(Plus, "plus")
        Define(Minus, "minus")
        Define(Multiply, "multiply")
        Define(Divide, "divide")
        Define(Modulus, "modulus")
        Define(ShiftRight, "shift right")
        Define(ShiftLeft, "shift left")
		Define(Unknown, "unknown")
		Define(Variable, "variable")
        Define(Name, "name")
        Define(IDSZ, "idsz")
        Define(NumericLiteral, "numeric literal")
        Define(Reference, "reference")
    #undef Define
	};
	return os;
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
	os << "token {";
	os << "location = " << token.getLocation().getLoadName() << ":" << token.getLocation().getLineNumber() << "," << std::endl;
	os << "value = " << token.getValue() << "," << std::endl;
	os << "type = " << token.getType() << "," << std::endl;
	os << "text = " << token.getText() << std::endl;
	os << "}" << std::endl;
	return os;
}
