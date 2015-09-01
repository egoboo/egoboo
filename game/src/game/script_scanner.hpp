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
	/**
	 * @brief
	 *	The line number.
	 */
	int _line;

	/**
	 * @brief
	 *	The index.
	 * @todo
	 *	MH: To whoever wrote this: This is what documentation is not.
	 */
    int _index;

	/**
	 * @brief
	 *	The integer value.
	 * @todo
	 *	MH: To whoever wrote this: This is what documentation is not.
	 */
    int _value;

    size_t szWord_length;
    STRING szWord;                   ///< The text representation

	/**
	 * @brief
	 *  Get the index of this token.
	 * @return
	 *  the index of this token
	 */
	int getIndex() const {
		return _index;
	}

	/**
	 * @brief
	 *  Set the index of this token.
	 * @param index
	 *  the index of this token
	 * @see getIndex
	 */
	void setIndex(int index) {
		_index = index;
	}

	/**
	 * @brief
	 *  Get the value of this token.
	 * @return
	 *  the value of this token
	 */
	int getValue() const {
		return _value;
	}

	/**
	 * @brief
	 *  Set the value of this token.
	 * @param value
	 *  the value of this token
	 * @see getValue
	 */
	void setValue(int value) {
		_value = value;
	}

	/**
	 * @brief
	 *  Get the line number of this token.
	 * @return
	 *  the line number of this token
	 * @remark
	 *  The line number is the line number of the line in which the lexeme of this token starts in.
	 */
	int getLine() const {
		return _line;
	}

	/**
	 * @brief
	 *  Set the line number of this token.
	 * @param line
	 *  the line number of this token
	 * @see getLine
	 */
	void setLine(int line) {
		_line = line;
	}

	/**
	 * @brief
	 *  Get the type of this token.
	 * @return
	 *  the type of this token
	 */
	Type getType() const {
		return _type;
	}

	/**
	 * @brief
	 *  Set the type of this token.
	 * @param type
	 *  the type of this token
	 */
	void setType(Type type) {
		_type = type;
	}

	/**
	 * @brief
	 *  Get the length of the lexeme of this token.
	 * @return
	 *  the length of the lexeme of this token
	 */
	size_t length() const {
		return szWord_length;
	}

	/**
	 * @brief
	 *  Construct this token with default values.
	 */
	Token();

	/**
	 * @brief
	 *  Construct this token with values of another token.
	 * @param other
	 *  the other token
	 */
	Token(const Token& other);

	/**
	 * @brief
	 *  Destruct this token.
	 */
	~Token();

	friend std::ostream& operator<<(std::ostream& os, const Token& token);
};

/**
 * @brief
 *  Overloaded &lt;&lt; operator for a token type.
 * @param ostream
 *  the output stream to write to
 * @param tokenType
 *  the token type to write
 */
std::ostream& operator<<(std::ostream& os, const Token::Type& tokenType);

/**
 * @brief
 *  Overloaded &lt;&lt; operator for a token.
 * @param ostream
 *  the output stream to write to
 * @param token
 *  the token to write
 */
std::ostream& operator<<(std::ostream& os, const Token& token);
