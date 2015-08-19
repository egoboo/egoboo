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
#pragma once

#include "game/egoboo_typedef.h"

#define MAX_OPCODE 1024 ///< Number of lines in AICODES.TXT

/**
 * @brief
 *	A token.
 */
struct Token {
	enum class Type {
		Unknown = '?',
		Function = 'F',
		Variable = 'V',
		Constant = 'C',
		Operator = 'O',
	};

private:
	/**
	 * @brief
	 *	The type of this token.
	 */
	Type _type;

public:

    int iLine;                       ///< Line number
    int iIndex;

    int iValue;                      ///< Integer value

    size_t szWord_length;
    STRING szWord;                   ///< The text representation

	/**
	 * @brief
	 *	Get the type of this token.
	 * @return
	 *	the type of this token
	 */
	Type getType() const {
		return _type;
	}

	/**
	 * @brief
	 *	Set the type of this token.
	 * @param type
	 *	the type of this token
	 */
	void setType(Type type) {
		_type = type;
	}

	/**
	 * @brief
	 *	Get the length of the lexeme of this token.
	 * @return
	 *	the length of the lexeme of this token
	 */
	size_t length() const {
		return szWord_length;
	}

	Token();
	Token(const Token& other);
	~Token();

};
