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

/// @file egolib/egolib_typedef.c
/// @brief Implementation of the support functions for Egoboo's special datatypes
/// @details

#include "egolib/typedef.h"

#include "egolib/log.h"
#include "egolib/_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void va_non_fatal_assert( const char *format, va_list args );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
const char * undo_idsz( IDSZ idsz )
{
    /// @author ZZ
    /// @details This function takes an integer and makes a text IDSZ out of it.

    static char value_string[5] = {"NONE"};

    if ( idsz == IDSZ_NONE )
    {
        strncpy( value_string, "NONE", SDL_arraysize( value_string ) );
    }
    else
    {
        // Bad! both function return and return to global variable!
        value_string[0] = (( idsz >> 15 ) & 0x1F ) + 'A';
        value_string[1] = (( idsz >> 10 ) & 0x1F ) + 'A';
        value_string[2] = (( idsz >> 5 ) & 0x1F ) + 'A';
        value_string[3] = (( idsz ) & 0x1F ) + 'A';
        value_string[4] = 0;
    }

    return value_string;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool irect_point_inside(irect_t * prect, int   ix, int   iy)
{
    if ( NULL == prect ) return false;

	/* @todo Remove +1. */
    if ( ix < prect->left || ix > prect->right  + 1 ) return false;
	/* @todo Remove +1. */
    if ( iy < prect->top  || iy > prect->bottom + 1 ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool frect_point_inside(frect_t * prect, float fx, float fy)
{
    if ( NULL == prect ) return false;

    if ( fx < prect->left || fx > prect->right ) return false;
    if ( fy < prect->top  || fy > prect->bottom ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void latch_init( latch_t * platch )
{
    if ( NULL == platch ) return;

    platch->x = 0.0f;
    platch->y = 0.0f;
    platch->b = EMPTY_BIT_FIELD;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void pair_to_range( IPair pair, FRange * prange )
{
    /// @author ZZ
    /// @details convert from a pair to a range

    if ( pair.base < 0 )
    {
        log_warning( "We got a randomization error again! (Base is less than 0)\n" );
    }

    if ( pair.rand < 0 )
    {
        log_warning( "We got a randomization error again! (rand is less than 0)\n" );
    }

    if ( NULL != prange )
    {
        float fFrom, fTo;

        fFrom = FP8_TO_FLOAT( pair.base );
        fTo   = FP8_TO_FLOAT( pair.base + pair.rand );

        prange->from = std::min( fFrom, fTo );
        prange->to   = std::max( fFrom, fTo );
    }
}

//--------------------------------------------------------------------------------------------
void range_to_pair( FRange range, IPair * ppair )
{
    /// @author ZZ
    /// @details convert from a range to a pair

    if ( range.from > range.to )
    {
        log_warning( "We got a range error! (to is less than from)\n" );
    }

    if ( NULL != ppair )
    {
        float fFrom, fTo;

        fFrom = std::min( range.from, range.to );
        fTo   = std::max( range.from, range.to );

        ppair->base = FLOAT_TO_FP8( fFrom );
        ppair->rand = FLOAT_TO_FP8( fTo - fFrom );
    }
}

//--------------------------------------------------------------------------------------------
void ints_to_range( int ibase, int irand, FRange * prange )
{
    IPair pair_tmp;

    pair_tmp.base = ibase;
    pair_tmp.rand = irand;

    pair_to_range( pair_tmp, prange );
}

//--------------------------------------------------------------------------------------------
void floats_to_pair( float vmin, float vmax, IPair * ppair )
{
    FRange range_tmp;

    range_tmp.from = vmin;
    range_tmp.to   = vmax;

    range_to_pair( range_tmp, ppair );
}

//--------------------------------------------------------------------------------------------
void va_non_fatal_assert( const char *format, va_list args )
{
    static char buffer[1024];

    vsnprintf( buffer, SDL_arraysize( buffer ), format, args );

    fputs( buffer, stderr );
}

//--------------------------------------------------------------------------------------------
void non_fatal_assert( bool val, const char * format, ... )
{
    va_list args;

    va_start( args, format );

    if ( !val )
    {
        va_non_fatal_assert( format, args );
    }

    va_end( args );
}
