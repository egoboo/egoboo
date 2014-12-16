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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/endian.h

#include <stdio.h>

#include <SDL_endian.h>

#if defined(_APPLE_)
#    include <Endian.h>
#endif

#include "egolib/typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// REMAPPING OF SDL MACROS
//--------------------------------------------------------------------------------------------

//---- conversion from the byteorder in ego files to the byteorder for this system

// define a ENDIAN_TO_SYS_IEEE32() "function" to work on both big and little endian systems
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    extern float ENDIAN_TO_SYS_IEEE32( float X );
#else
#    define ENDIAN_TO_SYS_IEEE32( X ) ( X )
#endif

#define ENDIAN_TO_SYS_INT08(X) (X)
#define ENDIAN_TO_SYS_INT16(X) SDL_SwapLE16(X)
#define ENDIAN_TO_SYS_INT32(X) SDL_SwapLE32(X)
#define ENDIAN_TO_SYS_INT64(X) SDL_SwapLE64(X)

//---- conversion from the byteorder for this system to the byteorder in ego files
//---- ( just repeat the process )

#define ENDIAN_TO_FILE_IEEE32(X) ENDIAN_TO_SYS_IEEE32(X)
#define ENDIAN_TO_FILE_INT08(X) ENDIAN_TO_SYS_INT08(X)
#define ENDIAN_TO_FILE_INT16(X) ENDIAN_TO_SYS_INT16(X)
#define ENDIAN_TO_FILE_INT32(X) ENDIAN_TO_SYS_INT32(X)
#define ENDIAN_TO_FILE_INT64(X) ENDIAN_TO_SYS_INT64(X)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    // automatic endian adjustment to fread basic types
    size_t endian_fread_ieee32( FILE* fileread, float * val );
    size_t endian_fread_uint64( FILE* fileread, Uint64 * val );
    size_t endian_fread_uint32( FILE* fileread, Uint32 * val );
    size_t endian_fread_uint16( FILE* fileread, Uint16 * val );
    size_t endian_fread_uint08( FILE* fileread, Uint8  * val );
    size_t endian_fread_sint64( FILE* fileread, Sint64 * val );
    size_t endian_fread_sint32( FILE* fileread, Sint32 * val );
    size_t endian_fread_sint16( FILE* fileread, Sint16 * val );
    size_t endian_fread_sint08( FILE* fileread, Sint8  * val );

    // automatic endian adjustment to fwrite basic types
    size_t endian_fwrite_ieee32( FILE* filewrite, float  val );
    size_t endian_fwrite_uint64( FILE* filewrite, Uint64 val );
    size_t endian_fwrite_uint32( FILE* filewrite, Uint32 val );
    size_t endian_fwrite_uint16( FILE* filewrite, Uint16 val );
    size_t endian_fwrite_uint08( FILE* filewrite, Uint8  val );
    size_t endian_fwrite_sint64( FILE* filewrite, Sint64 val );
    size_t endian_fwrite_sint32( FILE* filewrite, Sint32 val );
    size_t endian_fwrite_sint16( FILE* filewrite, Sint16 val );
    size_t endian_fwrite_sint08( FILE* filewrite, Sint8  val );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_endian_h
