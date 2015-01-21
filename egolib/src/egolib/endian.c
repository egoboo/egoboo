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

/// @file egolib/endian.c
/// @brief Implementation of endian conversion routines
/// @details

#include "egolib/endian.h"
#include "egolib/vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if SDL_BYTEORDER != SDL_LIL_ENDIAN

union u_ieee32_convert {float f; Uint32 i;};
typedef union u_ieee32_convert ieee32_convert_t;

union u_ieee64_convert {double f; Uint64 i;};
typedef union u_ieee64_convert ieee64_convert_t;

//--------------------------------------------------------------------------------------------
float ENDIAN_TO_SYS_IEEE32( float X )
{
    ieee32_convert_t utmp;

    utmp.f = X;

    utmp.i = SDL_SwapLE32( utmp.i );

    return utmp.f;
}

//--------------------------------------------------------------------------------------------
float ENDIAN_TO_SYS_IEEE64( double X )
{
    ieee64_convert_t utmp;

    utmp.f = X;

    utmp.i = SDL_SwapLE64( utmp.i );

    return utmp.f;
}

#endif
