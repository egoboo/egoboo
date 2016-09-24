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

/// @file   IdLib/AssertionFailedException.hpp
/// @brief  Definition of an exception indicating that an assertion failed.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Exception.hpp"

namespace Id {

/**
 * @brief
 *  Exception for when an assertion failed.
 */
class AssertionFailedException : public Exception {

private:
        
    /**
     * @brief
     *  A string describing the assertion e.g. <tt>nullptr != ptr</tt>.
     */
    std::string _assertion;

public:

    /**
     * @brief
     *  Construct this exception.
     * @param file
     *  the C++ source file associated with this exception
     * @param line
     *  the line within the C++ source file associated with this exception
     * @param assertion
     *  a description of the assertion
     */
	AssertionFailedException(const char *file, int line, const string& assertion) :
        Exception(file, line), _assertion(assertion)
    {}
    
    /**
     * @brief
     *  Construct this exception with the value of another exception.
     * @param other
     *  the other exception
     */
	AssertionFailedException(const AssertionFailedException& other) :
        Exception(other), _assertion(other._assertion)
    {}
 
    /**
     * @brief
     *  Assign this exception the values of another exception.
     * @param other
     *  the other exception
     * @return
     *  this exception
     */
	AssertionFailedException& operator=(const AssertionFailedException& other)
    {
        Exception::operator=(other);
        _assertion = other._assertion;
        return *this;
    }

public:

    /**
     * @brief
     *  Get a description of the assertion.
     * @return
     *  a description of the assertion
     */
    const string& getAssertion() const {
        return _assertion;
    }

    virtual operator string() const override {
        ostringstream buffer;
        buffer << "assertion `" << _assertion << "` failed";
        buffer << " (raised in file " << getFile() << ", line " << getLine() << ")";
        return buffer.str();
    }

};

} // namespace Id
