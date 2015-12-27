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

/// @file egolib/Logic/TreasureTables.hpp

#pragma once

#include "egolib/typedef.h"
#include "egolib/fileutil.h"

namespace Ego
{

class TreasureTables
{
public:
	/// This loads all the treasure tables from <tt>randomtreasure.txt</tt>.
	TreasureTables(const std::string& filepath);

	/**
	 * @brief Search treasure tables for random treasure. Follows references, detects circles.
	 * @param treasureTableName the name of the treasure table to start searching
	 * @return @a the name (std::string) of the treasure item or an empty string if none was found
	 * @pre @a treasureTableName must be a valid treasure table name.
	 * @remarks A valid treasure table name starts with <tt>%</tt>.
	 * @post If @a true was returned, @a treasureName was assigned a treasure name.
	 * If @a false was returned, @a treasure name was assigned the empty string.
	 * @remarks Follows references, detects circles.
	 */
	 std::string getRandomTreasure(const std::string& treasureTableName) const;

	 //Parsing and Scanner stuff
private:
	struct Token {
	    enum class Type {
	        StartOfInput,
	        EndOfInput,
	        Reference,
	        Name,
	        EndOfTable,
	        StartOfTable,
	    };
	    Type type;
	    std::string text;
	    Token(Type type, const std::string& text) : type(type), text(text) {}
	};
	void parse(const std::vector<Token>& tks);
	bool is(Token::Type type) const;
	std::vector<Token> read(ReadContext& ctxt);
	Token readElement(ReadContext& ctxt);

private:
	std::unordered_map<std::string, std::vector<std::string>> _treasureTables;

    std::vector<Token>::const_iterator current, begin, end;
};

} //Ego