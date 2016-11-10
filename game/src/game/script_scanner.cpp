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
		case Token::Type::Constant:       os << "Constant";       break;
		case Token::Type::Function:       os << "Function";       break;
		case Token::Type::Operator:       os << "Operator";       break;
		case Token::Type::Unknown:        os << "Unknown" ;       break;
		case Token::Type::Variable:       os << "Variable";       break;
        case Token::Type::Name:           os << "Name";           break;
        case Token::Type::IDSZ:           os << "IDSZ";           break;
        case Token::Type::NumericLiteral: os << "NumericLiteral"; break;
        case Token::Type::Reference:      os << "Reference";      break;
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
