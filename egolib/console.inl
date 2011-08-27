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

// This is a silly file so that we can keep tin internals of the egolib_console_t hidden,
// while allowing lua_console to "inherit" the egolib_console_t structure

#include "../egolib/console.h"
#include "../egolib/font_ttf.h"

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

        Font * pfont;

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define egolib_console_inl
