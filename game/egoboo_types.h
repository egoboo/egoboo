/* Egoboo - egobootypedef.h
 * Defines some basic types that are used throughout the game code.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef Egoboo_egobootypedef_h
#define Egoboo_egobootypedef_h

#include "egoboo_config.h"
#include "egoboo_config.h"

#include <SDL_endian.h>
#include <SDL_types.h>

extern float LoadFloatByteswapped( float *ptr );

typedef struct rect_sint32_t
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
} IRect;

typedef struct rect_float_t
{
  float left;
  float right;
  float top;
  float bottom;
} FRect;


typedef enum bool_e
{
  btrue  = ( 1 == 1 ),
  bfalse = ( !btrue )
} bool_t;


typedef char STRING[256];

typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(idsz) ((IDSZ)((((idsz)[0]-'A') << 15) | (((idsz)[1]-'A') << 10) | (((idsz)[2]-'A') << 5) | (((idsz)[3]-'A') << 0)))
#endif


typedef struct pair_t
{
  Sint32 ibase;
  Uint32 irand;
} PAIR;

typedef struct range_t
{
  float ffrom, fto;
} RANGE;

typedef Uint16 CHR_REF;
typedef Uint16 TEAM_REF;
typedef Uint16 PRT_REF;
typedef Uint16 PLA_REF;


#endif // include guard

