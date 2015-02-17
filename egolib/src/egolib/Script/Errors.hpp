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
         *  An exception to indicate a (generic) lexical error in a file.
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

        protected:

            std::ostringstream& writeLocation(std::ostringstream& o) const;

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
             */
            LexicalError(const char *file, int line, const Location& location);

            /**
             * @brief
             *  Get the location associated with this error.
             * @return
             *  the location associated with this error
             */
            const Location& getLocation() const;

            /**
             * @brief
             *  Overloaded cast to std::string operator.
             * @return
             *  the result of the cast
             */
            operator std::string() const override;

        };

        /**
         * @brief
         *  An exception to indicate a missing delimiter error in a file.
         * @author
         *  Michael Heilmann
         */
        class MissingDelimiterError : public LexicalError
        {

        private:

            /**
             * @brief
             *  The expected delimiter.
             */
            char _delimiter;

        public:

            /**
             * @brief
             *  Construct a missing delimiter error.
             * @param file, line, location
             *  see documentation of Ego::Script::LexicalError(const char *,int,const Ego::Script::Location&)
             * @param delimiter
             *  the expected delimiter
             */
            MissingDelimiterError(const char *file, int line, const Location& location, char delimiter);

            operator std::string() const override;

        };

    }
}