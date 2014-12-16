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

/// @file egolib/console.h
/// @A quake-console that can be used for anything.

#include "egolib/typedef.h"

#include <SDL.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

    // opaque console struct
    struct s_egolib_console;
    typedef struct s_egolib_console egolib_console_t;

    /// console callback used to implement specializations of the egolib_console
    typedef SDL_bool( *egolib_console_callback_t )( egolib_console_t * pcon, void * data );

//--------------------------------------------------------------------------------------------
// struct s_egolib_console
//--------------------------------------------------------------------------------------------

    egolib_console_t * egolib_console_create( egolib_console_t * pcon, SDL_Rect Con_rect, egolib_console_callback_t pcall, void * data );
    SDL_bool           egolib_console_destroy( egolib_console_t ** pcon, SDL_bool do_free );
    void               egolib_console_show( egolib_console_t * pcon );
    void               egolib_console_hide( egolib_console_t * pcon );
    void               egolib_console_fprint( egolib_console_t * pcon, const char *format, ... );

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    void        egolib_console_begin( void );
    void        egolib_console_end( void );
    void        egolib_console_draw_all( void );

    SDL_Event * egolib_console_handle_events( SDL_Event * evt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_console_h
