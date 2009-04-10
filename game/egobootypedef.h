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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - egobootypedef.h
 * Defines some basic types that are used throughout the game code.
 */

#include "egoboo_config.h"

#include <SDL_types.h>

//RECTANGLE
typedef struct s_rect
{
    Sint32 left;
    Sint32 right;
    Sint32 top;
    Sint32 bottom;
} rect_t;

//BOOLEAN
typedef char bool_t;
enum
{
    btrue = ( 1 == 1 ),
    bfalse = ( !btrue )
};

//BYTES
typedef int8_t    sbyte;
typedef uint8_t   byte;

//IDSZ
typedef Uint32 IDSZ;

extern char idsz_string[5];

#ifndef Make_IDSZ
#define Make_IDSZ(string) ((IDSZ)((((string)[0]-'A') << 15) | (((string)[1]-'A') << 10) | (((string)[2]-'A') << 5) | (((string)[3]-'A') << 0)))
#endif

#define IDSZ_NONE            Make_IDSZ("NONE")       // [NONE]

//STRING
typedef char STRING[256];

//FAST CONVERSIONS
#define FP8_TO_FLOAT(XX)   ( (float)(XX)/(float)0xFF )
#define FLOAT_TO_FP8(XX)   ( (Uint32)((XX)*(float)0xFF) )
#define FP8_TO_INT(XX)     ( (XX) >> 8 )                      // fast version of XX / 256
#define INT_TO_FP8(XX)     ( (XX) << 8 )                      // fast version of XX * 256
#define FP8_MUL(XX, YY)    ( ((XX)*(YY)) >> 8 )
#define FP8_DIV(XX, YY)    ( ((XX)<<8) / (YY) )

//AI targeting
typedef enum target_type
{
    ENEMY = 0,
    FRIEND,
    ALL,
    NONE
} TARGET_TYPE;

#define Egoboo_egobootypedef_h
