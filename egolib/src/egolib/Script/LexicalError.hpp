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

#include "egolib/Script/AbstractLexicalError.hpp"

namespace Ego
{
namespace Script
{

using namespace std;

/**
 * @brief
 *  An exception for generic lexical errors.
 * @author
 *  Michael Heilmann
 */
class LexicalError : public AbstractLexicalError
{

private:

    /**
     * @brief
     *  A message describing the error.
     */
    string _message;

public:

    /**
     * @brief
     *  Construct this exception.
     * @param file
     *  the C++ source file name associated with this error
     * @param line
     *  the line within the C++ source file associated with this error
     * @param location
     *  the location associated with this error
     *  the load name of the file associated with this error
     * @param message
     *  a message describing the error
     */
    LexicalError(const char *file, int line, const Location& location, const string& message) :
        AbstractLexicalError(file, line, location), _message(message)
    {}
    LexicalError(const LexicalError& other) :
        AbstractLexicalError(other), _message(other._message)
    {}
    LexicalError& operator=(const LexicalError& other)
    {
        AbstractLexicalError::operator=(other);
        _message = other._message;
        return *this;
    }

    /**
     * @brief
     *  Overloaded cast to std::string operator.
     * @return
     *  the result of the cast
     */
    operator string() const override
    {
        ostringstream o;
        writeLocation(o);
        o << " - "
            << "lexical error: "
            << _message;
        ;
        return o.str();
    }

};

} // namespace Script
} // namespace Ego
