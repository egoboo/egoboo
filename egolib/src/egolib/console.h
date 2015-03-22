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

/// @file  egolib/console.h
/// @brief A quake-style console that can be used for anything.

#pragma once

#include "egolib/typedef.h"

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
    void               egolib_console_fprint( egolib_console_t * pcon, const char *format, ... ) GCC_PRINTF_FUNC( 2 );

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    void        egolib_console_begin();
    void        egolib_console_end();
    void        egolib_console_draw_all();

    SDL_Event * egolib_console_handle_events( SDL_Event * evt );


/// @todo: Remove when egolib_console_t is a proper C++ class
class egolib_console_FontWrapper;

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EGOBOO_CONSOLE_LINES   32
#define EGOBOO_CONSOLE_LENGTH 256
#define EGOBOO_CONSOLE_PROMPT '>'
#define EGOBOO_CONSOLE_OUTPUT 4096

#define EGOBOO_CONSOLE_WRITE_LEN 1024

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The encapsulation of the data necessary to run a generic Quake-like console in Egoboo
    struct s_egolib_console
    {
        egolib_console_t           * pnext;

        egolib_console_callback_t    run_func;
        void                       * run_data;

        egolib_console_FontWrapper * pfont;

        SDL_bool on;

        SDL_Rect rect;

        int    save_count;
        int    save_index;
        char   save_buffer[EGOBOO_CONSOLE_LINES][EGOBOO_CONSOLE_LENGTH];

        size_t buffer_carat;
        char   buffer[EGOBOO_CONSOLE_LENGTH];

        size_t output_carat;
        char   output_buffer[EGOBOO_CONSOLE_OUTPUT];
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern egolib_console_t * egolib_console_top;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif
