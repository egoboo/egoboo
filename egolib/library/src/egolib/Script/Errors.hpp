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
#include "idlib/hll.hpp"

namespace Ego {
namespace Script {

/// @brief An exception to indicate a missing delimiter lexical error.
/// @author Michael Heilmann
class MissingDelimiterError : public idlib::hll::compilation_error
{
private:
    /// @brief The expected delimiter.
    char delimiter;

public:
    /// @brief Construct a missing delimiter error.
    /// @param file the C++ source file (as obtained by the __FILE__ macro) associated with this exception
    /// @param line the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
    /// @param delimiter the expected delimiter
    MissingDelimiterError(const char *file, int line, const idlib::hll::location& location, char delimiter) :
        idlib::hll::compilation_error(file, line, idlib::hll::compilation_error_kind::lexical, location, std::string("missing delimiter `") + delimiter + "`"),
        delimiter(delimiter)
    {}
};

} // namespace Script
} // namespace Ego
