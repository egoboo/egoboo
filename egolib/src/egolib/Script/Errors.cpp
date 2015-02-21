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
#include "egolib/Script/Errors.hpp"

namespace Ego
{
    namespace Script
    {
    
        LexicalError::LexicalError(const char *file, int line, const Location& location) :
            Ego::Exception(file, line), _location(location)
        {}

        const Location& LexicalError::getLocation() const
        {
            return _location;
        }

        LexicalError::operator std::string() const
        {
            std::ostringstream o;
            writeLocation(o);
            o << " - "
                << "<generic lexical error>"
                ;
            return o.str();
        }

        std::ostringstream& LexicalError::writeLocation(std::ostringstream& o) const
        {
            o << _location.getLoadName() << ": " << _location.getLineNumber()
                << " (raised in file " << getFile() << ", line " << getLine() << ")";
            return o;
        }

        SyntaxError::SyntaxError(const char *file, int line, const Location& location) :
            Ego::Exception(file, line), _location(location)
        {}

        const Location& SyntaxError::getLocation() const
        {
            return _location;
        }

        SyntaxError::operator std::string() const
        {
            std::ostringstream o;
            writeLocation(o);
            o << " - "
                << "<generic syntax error>"
                ;
            return o.str();
        }

        std::ostringstream& SyntaxError::writeLocation(std::ostringstream& o) const
        {
            o << _location.getLoadName() << ": " << _location.getLineNumber()
                << " (raised in file " << getFile() << ", line " << getLine() << ")";
            return o;
        }

        MissingDelimiterError::MissingDelimiterError(const char *file, int line, const Location& location, char delimiter) :
            LexicalError(file, line, location), _delimiter(delimiter)
        {}

        MissingDelimiterError::operator std::string()  const
        {
            std::ostringstream o;
            writeLocation(o);
            o << " - "
                << "missing delimiter `" << _delimiter << "`"
                ;
            return o.str();
        }

    }
}