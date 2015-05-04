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

#include "egolib/platform.h"

namespace Ego
{
namespace Core
{

using namespace std;

/**
 * @brief
 *  The base class of all IdLib (subsequently EgoLib, Cartman and Egoboo) exceptions.
 * @author
 *  Michael Heilmann
 */
class Exception
{

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
     *  Construct this IdLib exception.
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
    Exception(const Exception& other) throw() :
        _file(other._file), _line(other._line)
    {}
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
    const char *getFile() const throw()
    {
        return _file;
    }

    /**
     * @brief
     *  Get the line within the C++ source file associated with this exception.
     * @return
     *  the line within the C++ source file associated with this exception
     */
    int getLine() const throw()
    {
        return _line;
    }

    /**
     * @brief
     *  Overloaded cast operator for casting into std::string.
     * @return
     *  a human-readable textual description of the string.
     */
    virtual operator string() const = 0;
};

/**
 * @brief
 *  The base class of all assertion failed exceptions.
 * @author
 *  Michael Heilmann
 */
class AssertionFailed : public Exception
{

private:
        
    /**
     * @brief
     *  A string describing the assertion e.g. <tt>nullptr != ptr</tt>.
     */
    string _assertion;

public:

    /**
     * @brief
     *  Construct this assertion failed (exception).
     * @param file
     *   the C++ source file associated with this exception
     * @param line
     *   the line within the C++ source file associated with this exception
     * @param assertion
     *   a description of the assertion
     */
    AssertionFailed(const char *file, int line, const string& assertion) :
        Exception(file, line), _assertion(assertion)
    {}
    AssertionFailed(const AssertionFailed& other) :
        Exception(other), _assertion(other._assertion)
    {}
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

// Raise an assertion.
#if defined(_DEBUG)
#define EGO2_ASSERT(assertion, ...) \
    if(!(assertion)) \
    { \
        throw Ego::Core::AssertionFailed(__FILE__, __LINE__, #assertion); \
    }
#else
#define EGO2_ASSERT(assertion) /* Empty statement. */;
#endif


