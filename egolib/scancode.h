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

/// @file scancode.h
/// @details translation of SDL scancodes to keys

#include <SDL/SDL.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// EXTERNAL VARIABLES
//--------------------------------------------------------------------------------------------

    // the fgetc(), toupper() and other character-related functions return int values...
    extern int  scancode_to_ascii[SDLK_LAST];
    extern int  scancode_to_ascii_shift[SDLK_LAST];

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    void scancode_begin( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_scancode_h
