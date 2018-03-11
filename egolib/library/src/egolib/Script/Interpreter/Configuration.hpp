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

/// @file   egolib/Script/Interpreter/Configuration.hpp
/// @brief  The configuration of the interpreter.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

#define Ego_Script_Interpreter_WithProfileRefs (1)

#if !defined(Ego_Script_Interpreter_WithProfileRefs)
#error(Ego_Script_Interpreter_WithProfileRefs is not defined)
#endif

namespace Ego {
namespace Script {
namespace Interpreter {

/**
 * @brief Enable implicit conversions of numeric types such that \f$Integer\f$
 * and \f$Real\f$ can be converted into each other.
 */
static constexpr bool WithImplicitConversions = true;

/**
 * @brief The C++ type representing a \f$Real\f$ value.
 * @invariant Must be either float or double.
 */
using RealValue = float;

/**
 * @brief The C++ type representing a \f$Integer\f$ value.
 * @invariant Must be either int or long.
 */
using IntegerValue = int;

} // namespace Interpreter
} // namespace Script
} // namespace Ego
