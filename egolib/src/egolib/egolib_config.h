#pragma once

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

/// @file egolib/egolib_config.h
/// @brief Compile switches

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// compliation flags

#undef  USE_LUA_CONSOLE       ///< LUA support for the console

#define LOG_TO_CONSOLE        ///< dump all log info to file and to the console. Only useful if your compiler generates console for program output. Otherwise the results will end up in a file called stdout.txt

/// How much script debugging.
///    0 -- debugging off ( requires defined(_DEBUG) )
/// >= 1 -- Log the amount of script time that every object uses (requires defined(_DEBUG) and DEBUG_PROFILE)
/// >= 2 -- Log the amount of time that every single script command uses (requires defined(_DEBUG) and DEBUG_PROFILE)
/// >= 3 -- decompile every script (requires defined(_DEBUG))
#define DEBUG_SCRIPT_LEVEL 0

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// do the includes last so that the compile switches are always set
#include "egolib/platform.h"
#include "egolib/endian.h"

//--------------------------------------------------------------------------------------------

#define _egolib__config_h_
