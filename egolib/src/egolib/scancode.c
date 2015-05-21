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

/// @file egolib/scancode.c
/// @brief
/// @details

#if 0

#include "egolib/scancode.h"
#include "egolib/strutil.h" /**< @todo Remove this include. */
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/log.h"

int scancode_to_ascii[SDLK_LAST];
int scancode_to_ascii_shift[SDLK_LAST];

void scancode_begin()
{
    // Do the basic translation.
    for (int src = 0; src < SDLK_LAST; src++)
    {
        // SDL uses ASCII values for it's virtual scancodes.
        scancode_to_ascii[src] = src;

        // Find the shifted value using char_toupper().
        int dst = src;
        if (src < 255)
        {
            dst = Ego::toupper((char)src);

            if (dst > 255)
            {
                /// @todo Use log error.
                log_error("%s:%d: char_topper() returned an out of range value `%d` for `%d`\n", \
                          __FILE__, __LINE__, dst, src);
            }
        }

        scancode_to_ascii_shift[src] = dst;
    }

    // Fix the keymap.
    scancode_to_ascii_shift[SDLK_1]  = '!';
    scancode_to_ascii_shift[SDLK_2]  = '@';
    scancode_to_ascii_shift[SDLK_3]  = '#';
    scancode_to_ascii_shift[SDLK_4]  = '$';
    scancode_to_ascii_shift[SDLK_5]  = '%';
    scancode_to_ascii_shift[SDLK_6]  = '^';
    scancode_to_ascii_shift[SDLK_7]  = '&';
    scancode_to_ascii_shift[SDLK_8]  = '*';
    scancode_to_ascii_shift[SDLK_9]  = '(';
    scancode_to_ascii_shift[SDLK_0]  = ')';

    scancode_to_ascii_shift[SDLK_QUOTE]        = '\"';
    scancode_to_ascii_shift[SDLK_SEMICOLON]    = ':';
    scancode_to_ascii_shift[SDLK_PERIOD]       = '>';
    scancode_to_ascii_shift[SDLK_COMMA]        = '<';
    scancode_to_ascii_shift[SDLK_BACKQUOTE]    = '~';
    scancode_to_ascii_shift[SDLK_MINUS]        = '_';
    scancode_to_ascii_shift[SDLK_EQUALS]       = '+';
    scancode_to_ascii_shift[SDLK_LEFTBRACKET]  = '{';
    scancode_to_ascii_shift[SDLK_RIGHTBRACKET] = '}';
    scancode_to_ascii_shift[SDLK_BACKSLASH]    = '|';
    scancode_to_ascii_shift[SDLK_SLASH]        = '?';
}

#endif
