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

/// @file   IdLib/AbstractLexicalError.hpp
/// @brief  Definition of an abstract exception indicating a lexical error
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Exception.hpp"
#include "IdLib/Location.hpp"

namespace Id {

using namespace std;

/**
 * @brief
 *  An abstract lexical error exception.
 */
class AbstractLexicalErrorException : public Exception
{

private:

    /**
     * @brief
     *  The location associated with this error.
     */
    Location _location;

protected:

    ostringstream& writeLocation(ostringstream& o) const {
        o << _location.getFileName() << ": " << _location.getLineNumber()
          << " (raised in file " << getFile() << ", line " << getLine() << ")";
        return o;
    }

    AbstractLexicalErrorException& operator=(const AbstractLexicalErrorException& other) {
        Exception::operator=(other);
        _location = other._location;
        return *this;
    }

    /**
     * @brief
     *  Construct this exception.
     * @param file
     *  the C++ source file name associated with this error
     * @param line
     *  the line within the C++ source file associated with this error
     * @param location
     *  the location associated with this error
     */
	AbstractLexicalErrorException(const char *file, int line, const Location& location) :
        Exception(file, line), _location(location)
    {}

public:

    /**
     * @brief
     *  Get the location associated with this error.
     * @return
     *  the location associated with this error
     */
    const Location& getLocation() const {
        return _location;
    }

};

} // namespace Id
