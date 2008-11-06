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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#ifndef Egoboo_egobootypedef_h
#define Egoboo_egobootypedef_h

#include "egoboo_config.h"

#include <SDL_types.h>

typedef struct s_rect
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
}rect_t;

typedef char bool_t;
enum
{
  btrue = ( 1 == 1 ),
  bfalse = ( !btrue )
};

#define Make_IDSZ(chA, chB, chC, chD) (((chA-'A')<<15)|((chB-'A')<<10)|((chC-'A')<<5)|(chD-'A'))




#endif // include guard

