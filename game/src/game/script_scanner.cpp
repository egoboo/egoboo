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
	: _text(), _location("<unknown>", 1), _index(MAX_OPCODE), _value(0), _type(Type::Unknown) 
{}

Token::Token(Type type, const Id::Location& location)
    : _text(), _location(location), _index(MAX_OPCODE), _value(0), _type(type)
{}

Token::Token(const Token& other)
	: _text(other._text),  _location(other._location), _index(other._index), _value(other._value), _type(other._type)
{}

Token::~Token()
{}

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
	os << "index = " << token.getIndex() << "," << std::endl;
	os << "type = " << token.getType() << "," << std::endl;
	os << "text = " << token.getText() << std::endl;
	os << "}" << std::endl;
	return os;
}
