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
	: szWord{ '\0' }, szWord_length(0),
	  _line(0), _index(MAX_OPCODE), _value(0), _type(Type::Unknown) {
}

Token::Token(const Token& other)
	: szWord{ '\0' }, szWord_length(other.szWord_length),
	  _line(other._line), _index(other._index), _value(other._value), _type(other._type) {
	strcpy(szWord, other.szWord);
}

Token::~Token() {
}

std::ostream& operator<<(std::ostream& os, const Token::Type& tokenType) {
	switch (tokenType) {
		case Token::Type::Constant: os << "Constant"; break;
		case Token::Type::Function: os << "Function"; break;
		case Token::Type::Operator: os << "Operator"; break;
		case Token::Type::Unknown:  os << "Unknown" ; break;
		case Token::Type::Variable: os << "Variable"; break;
	};
	return os;
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
	os << "token {";
	os << "line = " << token.getLine() << "," << std::endl;
	os << "value = " << token.getValue() << "," << std::endl;
	os << "index = " << token._index << "," << std::endl;
	os << "type = " << token.getType() << "," << std::endl;
	os << "word = " << token.szWord << std::endl;
	os << "}" << std::endl;
	return os;
}
