#pragma once

//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include <egolib.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#ifndef HAS_BITS
#    define HAS_BITS(A, B) ( 0 != ((A)&(B)) )
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef float cart_vec_t[3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t SDL_RectIntersect( SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect );
