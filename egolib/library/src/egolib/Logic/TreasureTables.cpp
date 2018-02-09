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

/// @file egolib/Logic/TreasureTable.hpp
/// @brief Implementation of code for handling random treasure generation
/// @details

#include "egolib/Logic/TreasureTables.hpp"

namespace Ego
{

TreasureTables::TreasureTables(const std::string& filePath)
{
	// Try to open a context.
	ReadContext ctxt(filePath);
	parse(read(ctxt));
}

std::string TreasureTables::getRandomTreasure(const std::string& treasureTableName) const
{
	static const std::string NO_TREASURE;

	// The empty string is not a valid treasure table name and treasure table names must start with '%'.
    if (treasureTableName.empty() || '%' != treasureTableName[0]) {
        return NO_TREASURE;
    }

    // To detect circular references, we keep the names of visited tables in this set.
    std::unordered_set<std::string> visited;

    // Make a local copy of the table name.
    std::string currentEntry = treasureTableName;

	// Keep searching until we have found something or abort searching for other reasons.
    while (true) {

        // If this table name was already visted ...
        if (visited.find(currentEntry) != visited.cend()) {
            // ... stop searching.
            break;
        }

        // Mark this table name as visited.
        visited.insert(currentEntry);

        // Find the table for the name
        const auto& result = _treasureTables.find(treasureTableName);

        // If the table does not exist or is empty ...
        if(result == _treasureTables.end() || result->second.empty()) {
            // ... stop searching.
        	break;
        }

    	//Pick a random element from the treasure table
        currentEntry = Random::getRandomElement(result->second);

        // If this is not a reference to yet another treasure table ...
        if ('%' != currentEntry[0]) {
            // ... an entry was found.
            return currentEntry;
        }

        // Otherwise we keep searching.
    }

    //Unable to generate a random treasure! :(
    return NO_TREASURE;
}

//-------------------------------------------------
//Scanning and Parser stuff below
//-------------------------------------------------

TreasureTables::Token TreasureTables::readElement(ReadContext& ctxt) {
    std::string text;
    if (ctxt.is('%'))
    {
        ctxt.next();
        text = '%' + ctxt.readName();
        return Token(Token::Type::Reference, text);
    }
    else
    {
        text = ctxt.readName();
        if (text == "END")
        {
            return Token(Token::Type::EndOfTable, text);
        }
        else
        {
            return Token(Token::Type::Name, text);
        }
    }
}

std::vector<TreasureTables::Token> TreasureTables::read(ReadContext& ctxt)
{
    std::vector<Token> tks;
    Token tk = Token(Token::Type::StartOfInput, "<start of input>");
    tks.push_back(tk);
    while (ctxt.skipToColon(true))
    {
        std::string name = ctxt.readName();
        tk = Token(Token::Type::StartOfTable, "%" + name);
        tks.push_back(tk);
        while (ctxt.skipToColon(false))
        {
            ctxt.skipWhiteSpaces();
            tk = readElement(ctxt);
            tks.push_back(tk);
            if (tk.type == Token::Type::EndOfTable)
            {
                break;
            }
        }
    }
    tk = Token(Token::Type::EndOfInput, "<end of input>");
    tks.push_back(tk);
    return tks;
}

bool TreasureTables::is(Token::Type type) const
{
    if (current == end) throw std::runtime_error("<internal error>");
    return (type == (*current).type);
}

// @code
// file = startOfInput table* endOfInput
// table = startOfTable (reference | name)* endOfTable
// @endcode
void TreasureTables::parse(const std::vector<Token>& tks)
{
    assert(!tks.empty()); // The token list is never empty.
    begin = tks.cbegin(); end = tks.cend(); current = begin;

    // `<start of input>`
    if (!is(Token::Type::StartOfInput))
    {
        throw std::runtime_error("syntax error");
    }
    ++current;

    while (is(Token::Type::StartOfTable))
    {
    	//Load next table name
    	std::vector<std::string>& table = _treasureTables[current->text];
    	++current;

        // `(reference>|name)*`
        while (is(Token::Type::Reference) || is(Token::Type::Name))
        {
            // action {
            table.push_back(current->text);
            // } 
            ++current;
        }
        // `endOfTable`
        if (!is(Token::Type::EndOfTable))
        {
            throw std::runtime_error("expected <end of table>");
        }
        ++current;
    }
    // `endOfInput`
    if (!is(Token::Type::EndOfInput))
    {
        throw std::runtime_error("expected <end of input>");
    }
}

} //Ego
