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

#include "egoboo_typedef.h"
#include <SDL_endian.h>

#if defined(_APPLE_)
#    include <Endian.h>
#endif

// define a ENDIAN_FLOAT() "function" to work on both big and little endian systems
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
extern float ENDIAN_FLOAT(float X);
#else
#    define ENDIAN_FLOAT( X ) ( X )
#endif

#define ENDIAN_INT08(X) SDL_SwapLE8(X)
#define ENDIAN_INT16(X) SDL_SwapLE16(X)
#define ENDIAN_INT32(X) SDL_SwapLE32(X)
#define ENDIAN_INT64(X) SDL_SwapLE64(X)
