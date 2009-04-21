#pragma once

// This is a silly file so that we can keep tin internals of the egoboo_console_t hidden,
// while allowing lua_console to "inherit" the egoboo_console_t structure

#include "egoboo_console.h"
#include "font.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EGOBOO_CONSOLE_LINES   32
#define EGOBOO_CONSOLE_LENGTH 256
#define EGOBOO_CONSOLE_PROMPT '>'
#define EGOBOO_CONSOLE_OUTPUT 4096

#define EGOBOO_CONSOLE_WRITE_LEN 1024

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

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

    int    buffer_carat;
    char   buffer[EGOBOO_CONSOLE_LENGTH];

    int    output_carat;
    char   output_buffer[EGOBOO_CONSOLE_OUTPUT];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern egoboo_console_t * egoboo_console_top;
