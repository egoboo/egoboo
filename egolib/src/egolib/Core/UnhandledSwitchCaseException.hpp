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
 *  Exception for when a switch is missing a case handle for a specific label
 * @author
 *  Johan Jansen
 */
class UnhandledSwitchCaseException : public Exception
{

public:

    /**
     * @brief
     *  Construct this environment error.
     * @param file
     *  the C++ source file (as obtained by the __FILE__ macro) associated with this exception
     * @param line
     *  the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
     * @param message
     * 	optional exception string message
     */
    UnhandledSwitchCaseException(const char *file, int line, const string& message = "Unhandled switch case") :
        Exception(file, line), 
        _message(message)
    {}
    UnhandledSwitchCaseException(const UnhandledSwitchCaseException& other) :
        Exception(other), 
        _message(other._message)
    {}
    UnhandledSwitchCaseException& operator=(const UnhandledSwitchCaseException& other)
    {
        Exception::operator=(other);
        _message = other._message;
        return *this;
    }

public:

    /**
     * @brief
     *  Get the message associated with this exception.
     * @return
     *  the message associated with this exception
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
        buffer << "(raised in file " << getFile() << ", line " << getLine() << ")"
               << ":" << std::endl;
        buffer << _message;
        return buffer.str();
    }

private:
	std::string _message;
};

} // namespace Core
} // namespace Ego