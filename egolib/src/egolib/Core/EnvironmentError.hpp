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

#include "egolib/Core/Exception.hpp"

namespace Ego
{
namespace Core
{

using namespace std;

/**
 * @brief
 *  A generic exception to propagate errors of the environment (e.g. SDL, Win32, OSX, Linux, ...)
 *  to the application level. Use this exception as well if the environment does not follow its
*   specification (e.g. documented behavior vs. actual behavior).
 * @author
 *  Michael Heilmann
 */
class EnvironmentError : public Exception
{

private:

    /**
     * @brief
     *  The component associated with this environment error.
     */
    string _component;

    /**
     * @brief
     *  The messsage associated with this environment error.
     */
    string _message;

public:

    /**
     * @brief
     *  Construct this environment error.
     * @param file
     *  the C++ source file (as obtained by the __FILE__ macro) associated with this exception
     * @param line
     *  the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
     * @param component
     *  the component the error is related to e.g. "pthreads", "SDL video", ...
     * @param message
     *  a message describing the error
     * @remark
     *  Intentionally protected.
     */
    EnvironmentError(const char *file, int line, const string& component, const string& message) :
        Exception(file, line), _component(component), _message(message)
    {}
    EnvironmentError(const EnvironmentError& other) :
        Exception(other), _component(other._component), _message(other._message)
    {}
    EnvironmentError& operator=(const EnvironmentError& other)
    {
        Exception::operator=(other);
        _component = other._component;
        _message = other._message;
        return *this;
    }

public:

    /**
     * @brief
     *  Get the component associated with this environment error.
     * @return
     *  the component associated with this environment error
     */
    const string& getComponent() const
    {
        return _component;
    }

    /**
     * @brief
     *  Get the message associated with this environment error.
     * @return
     *  the message associated with this environment error
     */
    const string& getMessage() const
    {
        return _message;
    }

    /**
     * @brief
     *  Overloaded cast operator for casting into std::string.
     * @return
     *  a human-readable textual description of the string.
     */
    virtual operator ::std::string() const
    {
        ostringstream buffer;
        buffer << _component
               << " "
               << "(raised in file " << getFile() << ", line " << getLine() << ")"
               << ":" << std::endl;
        buffer << _message;
        return buffer.str();
    }
};

} // namespace Core
} // namespace Ego