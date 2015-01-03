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

/// @file egolib/_math.inl
/// @brief
/// @details Almost all of the math functions are intended to be inlined for maximum speed

#pragma once

#include "egolib/_math.h"
#include "egolib/vec.h"
#include "egolib/platform.h"
#include "egolib/log.h"

#include "egolib/extensions/ogl_include.h"
#include "egolib/extensions/ogl_debug.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif



// conversion functions
    static INLINE FACING_T vec_to_facing( const float dx, const float dy );
    static INLINE void     facing_to_vec( const FACING_T facing, float * dx, float * dy );

// rotation functions
    static INLINE int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight );

// limiting functions
    static INLINE void getadd_int( const int min, const int value, const int max, int* valuetoadd );
    static INLINE void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    static INLINE int generate_irand_pair( const IPair num );
    static INLINE int generate_irand_range( const FRange num );
    static INLINE int generate_randmask( const int base, const Uint32 mask );

// matrix functions

#if defined(__cplusplus)
}

#endif


//--------------------------------------------------------------------------------------------
// CONVERSION FUNCTIONS
//--------------------------------------------------------------------------------------------

static INLINE FACING_T vec_to_facing( const float dx, const float dy )
{
    return ( FACING_T )( RAD_TO_FACING( ATAN2( dy, dx ) + PI ) );
}

//--------------------------------------------------------------------------------------------
static INLINE void facing_to_vec( const FACING_T facing, float * dx, float * dy )
{
    TURN_T turn = TO_TURN( facing - 0x8000 );

    if ( NULL != dx )
    {
        *dx = turntocos[turn];
    }

    if ( NULL != dy )
    {
        *dy = turntosin[turn];
    }
}

//--------------------------------------------------------------------------------------------
// ROTATION FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight )
{
    /// @author ZZ
    /// @details This function returns a direction between the major and minor ones, closer
    ///    to the major.

    int diff;

    // Align major direction with 0
    diff = ( int )minordir - ( int )majordir;

    if ( diff <= -( int )0x8000L )
    {
        diff += ( int )0x00010000L;
    }
    else if ( diff >= ( int )0x8000L )
    {
        diff -= ( int )0x00010000L;
    }

    return diff / weight;
}

//--------------------------------------------------------------------------------------------
// LIMITING FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE void getadd_int( const int min, const int value, const int max, int* valuetoadd )
{
    /// @author ZZ
    /// @details This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

    int newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
static INLINE void getadd_flt( const float min, const float value, const float max, float* valuetoadd )
{
    /// @author ZZ
    /// @details This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

    float newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
// RANDOM FUNCTIONS
//--------------------------------------------------------------------------------------------
static INLINE int generate_irand_pair( const IPair num )
{
    /// @author ZZ
    /// @details This function generates a random number

    int tmp;
    int irand = RANDIE;

    tmp = num.base;
    if ( num.rand > 1 )
    {
        tmp += irand % num.rand;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
static INLINE int generate_irand_range( const FRange num )
{
    /// @author ZZ
    /// @details This function generates a random number

    IPair loc_pair;

    range_to_pair( num, &loc_pair );

    return generate_irand_pair( loc_pair );
}

//--------------------------------------------------------------------------------------------
static INLINE int generate_randmask( const int base, const Uint32 mask )
{
    /// @author ZZ
    /// @details This function generates a random number

    int tmp;
    int irand = RANDIE;

    tmp = base;
    tmp += irand & mask;

    return tmp;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif
