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

/// @file egolib/Script/PDLTokenKind.cpp
/// @brief Token kinds of the PDL (Program Definition Language) of EgoScript.
/// @author Michael Heilmann

#include "egolib/Script/PDLTokenKind.hpp"
#include "idlib/idlib.hpp"

namespace Ego::Script {

std::string toString(PDLTokenKind kind)
{
    switch (kind)
    {
    #define Define(name, string) case PDLTokenKind::name: return string;
    #include "egolib/Script/PDLTokenKind.in"
    #undef Define
        default:
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__, "unknown token type");
    };
}

std::ostream& operator<<(std::ostream& os, const PDLTokenKind& kind)
{
    os << toString(kind);
    return os;
}

} // namespace Ego::Script
