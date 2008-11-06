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

#include "egobootypedef.h"

#include <SDL_endian.h>

// define a LoadFloatByteswapped() "function" to work on both big and little endian systems
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
extern float LoadFloatByteswapped( float *ptr );
#else
#    define LoadFloatByteswapped( PTR ) (NULL == PTR ? 0.0f : *PTR)
#endif