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

#include "egolib/Script/Location.hpp"
#include "egolib/Exception.hpp"

namespace Ego
{
    namespace Script
    {

        /**
         * @brief
         *  An exception to indicate a lexical error in a file.
         * @author
         *  Michael Heilmann
         */
        class LexicalError : public Exception
        {

        private:

            /**
             * @brief
             *  The location associated with this error.
             */
            Location _location;

        public:

            /**
             * @brief
             *  Construct a lexical error.
             * @param file
             *  the C++ source file name associated with this error
             * @param line
             *  the line within the C++ source file associated with this error
             * @param location
             *  the location associated with this error
             *  the load name of the file associated with this error
             * @param lineNumber
             *  the line number in the file asssociated with this error
             */
            LexicalError(const char *file, int line, const Location& location) :
                Ego::Exception(file, line), _location(location)
            {}

            /**
             * @brief
             *  Get the location associated with this error.
             * @return
             *  the location associated with this error
             */
            const Location& getLocation() const
            {
                return _location;
            }

            operator std::string() const override
            {
                std::ostringstream buffer;
                buffer << _location.getLoadName() << ": " << _location.getLineNumber() << ": " << "lexical error";
                buffer << " (raised in file " << getFile() << ", line " << getLine() << ")";
                return buffer.str();
            }

        };

    }
}