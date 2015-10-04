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

/// @file   IdLib/Exception.hpp
/// @brief  Root of the exception hierarchy.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Platform.hpp"

namespace Id {

using namespace std;

/**
 * @brief
 *  The base class of all Id exceptions.
 */
class Exception {

private:

    /**
     * @brief
     *  The C++ source file (as obtained by the __FILE__ macro) associated with this exception.
     */
    char *_file;

    /**
     * @brief
     *  The line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception.
     */
    int _line;

protected:

    /**
     * @brief
     *  Construct this exception.
     * @param file
     *  the C++ source file (as obtained by the __FILE__ macro) associated with this exception
     * @param line
     *  the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
     * @remark
     *  Intentionally protected.
     */
    Exception(const char *file, int line) throw() :
        _file((char *)(file)), _line(line)
    {}

    /**
     * @brief
     *  Construct this exception using the values of another exception.
     * @param other
     *  the other exception
     * @remark
     *  Intentionally protected.
     */
    Exception(const Exception& other) throw() :
        _file(other._file), _line(other._line)
    {}

    /**
     * @brief
     *  Assign this exception the values of another exception
     * @param other
     *  the other exception
     * @return
     *  this exception
     * @remark
     *  Intentionally protected
     */
    Exception& operator=(const Exception& other) throw()
    {
        _file = other._file;
        _line = other._line;
        return *this;
    }

    /**
     * @brief
     *  Destruct this IdLib exception.
     * @remark
     *  Intentionally protected.
     */
    virtual ~Exception()
    {}

public:

    /**
     * @brief
     *  Get the C++ source file associated with this exception.
     * @return
     *  the C++ source file associated with this exception
     */
    const char *getFile() const throw() {
        return _file;
    }

    /**
     * @brief
     *  Get the line within the C++ source file associated with this exception.
     * @return
     *  the line within the C++ source file associated with this exception
     */
    int getLine() const throw() {
        return _line;
    }

    /**
     * @brief
     *  Overloaded cast operator for casting into std::string.
     * @return
     *  a human-readable textual description of the string.
     */
	virtual operator string() const {
		std::ostringstream os;
		os << getFile() << ":" << getLine() << ": exception";
		return os.str();
	}
};

} // namespace Id
