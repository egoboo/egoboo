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

namespace Ego {
namespace Script {

using namespace Id;
using namespace std;

/**
 * @brief
 *  An exception to indicate a missing delimiter lexical error.
 * @author
 *  Michael Heilmann
 */
class MissingDelimiterError : public AbstractLexicalErrorException
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
     *  see documentation of Id::LexicalErrorException(const char *,int,const Id::Location&)
     * @param delimiter
     *  the expected delimiter
     */
    MissingDelimiterError(const char *file, int line, const Location& location, char delimiter) :
        AbstractLexicalErrorException(file, line, location), _delimiter(delimiter)
    {}

    operator string() const override {
        std::ostringstream o;
        writeLocation(o);
        o << " - "
            << "missing delimiter `" << _delimiter << "`"
            ;
        return o.str();
    }

};

} // namespace Script
} // namespace Ego
