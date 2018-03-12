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

/// @file   egolib/Script/Interpreter/Configuration.cpp
/// @brief  The configuration of the interpreter.
/// @author Michael Heilmann

#include "egolib/Script/Interpreter/Configuration.hpp"
#include <type_traits>

namespace Ego::Script::Interpreter {

static_assert(std::is_same<RealValue, float>::value || std::is_same<RealValue, double>::value,
              "Ego::Script::Interpreter::Real must be either float or double");

static_assert(std::is_same<IntegerValue, int>::value || std::is_same<IntegerValue, long>::value,
              "Ego::Script::Interpreter::Integer must be either int or long");

} // namespace Ego::Script::Interpreter
