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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

size_t endian_fread_ieee32( FILE* fileread, float * val )
{
    size_t retval;
    float val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_IEEE32( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------

size_t endian_fread_uint64( FILE* fileread, Uint64 * val )
{
    size_t retval;
    Uint64 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT64( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t endian_fread_uint32( FILE* fileread, Uint32 * val )
{
    size_t retval;
    Uint32 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT32( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t endian_fread_uint16( FILE* fileread, Uint16 * val )
{
    size_t retval;
    Uint16 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT16( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t endian_fread_uint08( FILE* fileread, Uint8  * val )
{
    size_t retval;
    Uint8 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT08( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------

size_t endian_fread_sint64( FILE* fileread, Sint64 * val )
{
    size_t retval;
    Sint64 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT64( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t endian_fread_sint32( FILE* fileread, Sint32 * val )
{
    size_t retval;
    Sint32 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT32( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t endian_fread_sint16( FILE* fileread, Sint16 * val )
{
    size_t retval;
    Sint16 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT16( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t endian_fread_sint08( FILE* fileread, Sint8  * val )
{
    size_t retval;
    Sint8 val_tmp;

    retval = fread( &val_tmp, sizeof( val_tmp ), 1, fileread );

    if ( retval > 0 && NULL != val )
    {
        *val = ENDIAN_TO_SYS_INT08( val_tmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
size_t endian_fwrite_ieee32( FILE* filewrite, float val )
{
    float val_tmp = ENDIAN_TO_SYS_IEEE32( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------
size_t endian_fwrite_uint64( FILE* filewrite, Uint64 val )
{
    Uint64 val_tmp = ENDIAN_TO_SYS_INT64( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------
size_t endian_fwrite_uint32( FILE* filewrite, Uint32 val )
{
    Uint32 val_tmp = ENDIAN_TO_SYS_INT32( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------
size_t endian_fwrite_uint16( FILE* filewrite, Uint16 val )
{
    Uint16 val_tmp = ENDIAN_TO_SYS_INT16( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------
size_t endian_fwrite_uint08( FILE* filewrite, Uint8  val )
{
    Uint8 val_tmp = ENDIAN_TO_SYS_INT08( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------

size_t endian_fwrite_sint64( FILE* filewrite, Sint64 val )
{
    Sint64 val_tmp = ENDIAN_TO_SYS_INT64( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------
size_t endian_fwrite_sint32( FILE* filewrite, Sint32 val )
{
    Sint32 val_tmp = ENDIAN_TO_SYS_INT32( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------
size_t endian_fwrite_sint16( FILE* filewrite, Sint16 val )
{
    Sint16 val_tmp = ENDIAN_TO_SYS_INT16( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}

//--------------------------------------------------------------------------------------------
size_t endian_fwrite_sint08( FILE* filewrite, Sint8  val )
{
    Sint8 val_tmp = ENDIAN_TO_SYS_INT08( val );

    return fwrite( &val_tmp, sizeof( val ), 1, filewrite );
}
