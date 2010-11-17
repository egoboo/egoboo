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

/// @file egoboo_console.h
/// @A quake-console that can be used for anything.

#include "egoboo_config.h"

#include <SDL.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

/// opaque console struct
    struct s_egoboo_console;
    typedef struct s_egoboo_console egoboo_console_t;

/// console callback used to implement specializations of the egoboo_console
    typedef SDL_bool( *egoboo_console_callback_t )( egoboo_console_t * pcon, void * data );

//--------------------------------------------------------------------------------------------
// struct s_egoboo_console
//--------------------------------------------------------------------------------------------

    egoboo_console_t * egoboo_console_create( egoboo_console_t * pcon, SDL_Rect Con_rect, egoboo_console_callback_t pcall, void * data );
    SDL_bool           egoboo_console_destroy( egoboo_console_t ** pcon, SDL_bool do_free );
    void egoboo_console_show( egoboo_console_t * pcon );
    void egoboo_console_hide( egoboo_console_t * pcon );

    void egoboo_console_fprint( egoboo_console_t * pcon, const char *format, ... );

//--------------------------------------------------------------------------------------------
// EXTERNAL VARIABLES
//--------------------------------------------------------------------------------------------

    extern Uint8  scancode_to_ascii[SDLK_LAST];
    extern Uint8  scancode_to_ascii_shift[SDLK_LAST];

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    void   init_scancodes();

    void egoboo_console_draw_all();

    SDL_Event * egoboo_console_handle_events( SDL_Event * evt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egoboo_console_h