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

// This is a silly file so that we can keep tin internals of the egoboo_console_t hidden,
// while allowing lua_console to "inherit" the egoboo_console_t structure

#include "egoboo_console.h"
#include "font_ttf.h"

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
struct s_egoboo_console
{
    egoboo_console_t           * pnext;

    egoboo_console_callback_t    run_func;
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

extern egoboo_console_t * egoboo_console_top;
