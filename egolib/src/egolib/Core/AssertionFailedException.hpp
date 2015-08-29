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

/// @file   egolib/Core/AssertionFailed.hpp
/// @brief  Definition of an exception indicating that an assertion failed.
/// @author Michael Heilmann

#pragma once

#include "egolib/Core/Exception.hpp"

namespace Ego {
namespace Core {

/**
 * @brief
 *  Exception for when an assertion failed.
 * @author
 *  Michael Heilmann
 */
class AssertionFailed : public Exception {

private:
        
    /**
     * @brief
     *  A string describing the assertion e.g. <tt>nullptr != ptr</tt>.
     */
    std::string _assertion;

public:

    /**
     * @brief
     *  Construct this assertion failed (exception).
     * @param file
     *  the C++ source file associated with this exception
     * @param line
     *  the line within the C++ source file associated with this exception
     * @param assertion
     *  a description of the assertion
     */
    AssertionFailed(const char *file, int line, const string& assertion) :
        Exception(file, line), _assertion(assertion)
    {}
    
    /**
     * @brief
     *  Construct this exception with the value of another exception.
     * @param other
     *  the other exception
     */
    AssertionFailed(const AssertionFailed& other) :
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
    AssertionFailed& operator=(const AssertionFailed& other)
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
    const string& getAssertion() const
    {
        return _assertion;
    }

    virtual operator string() const
    {
        ostringstream buffer;
        buffer << "assertion `" << _assertion << "` failed";
        buffer << " (raised in file " << getFile() << ", line " << getLine() << ")";
        return buffer.str();
    }

};

} // namespace Core
} // namespace Ego