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

/// @file scancode.c
/// @brief
/// @details

#include "../egolib/scancode.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint8  scancode_to_ascii[SDLK_LAST];
Uint8  scancode_to_ascii_shift[SDLK_LAST];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scancode_begin()
{
    /// @author BB
    /// @details initialize the scancode translation

    int i;

    // do the basic translation
    for ( i = 0; i < SDLK_LAST; i++ )
    {
        // SDL uses ascii values for it's virtual scancodes
        scancode_to_ascii[i] = i;
        if ( i < 255 )
        {
            scancode_to_ascii_shift[i] = toupper( i );
        }
        else
        {
            scancode_to_ascii_shift[i] = scancode_to_ascii[i];
        }
    }

    // fix the keymap
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
