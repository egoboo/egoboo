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

/// @file IdLib/SyntacticalErrorException.hpp
/// @brief Definition of an abstract exception indicating a syntactical error.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/AbstractSyntacticalErrorException.hpp"

namespace Id {

/// @brief An abstract lexical error exception.
class SyntacticalErrorException : public AbstractSyntacticalErrorException
{
protected:
    /// @brief A message describing the errror.
    string _message;

public:
    /// @brief Construct this exception.
    /// @param file the C++ source file (as obtained by the __FILE__ macro) associated with this exception
    /// @param line the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
    /// @param location the location associated with this error
    /// @param message a message describing the error
    SyntacticalErrorException(const char *file, int line, const Location& location, const std::string& message) :
        AbstractSyntacticalErrorException(file, line, location), _message(message)
    {}

    SyntacticalErrorException(const SyntacticalErrorException& other) :
        AbstractSyntacticalErrorException(other), _message(other._message)
    {}

    SyntacticalErrorException& operator=(const SyntacticalErrorException& other)
    {
        AbstractSyntacticalErrorException::operator=(other);
        _message = other._message;
        return *this;
    }

    /// @brief Overloaded cast to std::string operator.
    /// @return the result of the cast
    operator string() const override
    {
        ostringstream o;
        writeLocation(o);
        o << " - "
            << "syntactical error: "
            << _message;
        return o.str();
    }

};

} // namespace Id
