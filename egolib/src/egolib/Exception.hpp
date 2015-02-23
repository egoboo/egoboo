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
        const char *_file;

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
         *   the C++ source file (as obtained by the __FILE__ macro) associated with this exception
         * @param line
         *   the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
         * @remark
         *  Intentionally protected.
         */
        Exception(const char *file, int line) :
            _file(file), _line(line)
        {}

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
        const char *getFile() const
        {
            return _file;
        }

        /**
         * @brief
         *  Get the line within the C++ source file associated with this exception.
         * @return
         *  the line within the C++ source file associated with this exception
         */
        int getLine() const
        {
            return _line;
        }

        /**
         * @brief
         *  Overloaded cast operator for casting into std::string.
         * @return
         *  a human-readable textual description of the string.
         */
        virtual operator ::std::string() const = 0;

        //Make exceptions non-copyable
        Exception(const Exception& copy) = default;
        Exception& operator=(const Exception&) = delete;
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
        const char *_assertion;

    protected:

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
        AssertionFailed(const char *file, int line, const char *assertion) :
            Exception(file,line), _assertion(assertion)
        {}

    public:

        /**
        * @brief
        *  Get a description of the assertion.
        * @return
        *  a description of the assertion
        */
        const char *getAssertion() const
        {
            return _assertion;
        }

        virtual operator ::std::string() const
        {
            std::ostringstream buffer;
            buffer << "assertion `" << _assertion << "` failed";
            buffer << " (raised in file " << getFile() << ", line " << getLine() << ")";
            return buffer.str();
        }

        //Make exceptions non-copyable
        AssertionFailed(const AssertionFailed& copy) = default;
        AssertionFailed& operator=(const AssertionFailed&) = delete;
    };
};

#define EGOBOO_ASSERT_2(assertion) \
	if(!(assertion)) \
	{ \
		throw Ego::Exception(__FILE__,__LINE__," assertion " #assertion "` failed"); \
	}
