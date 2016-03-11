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

/// @file egolib/math.c
/// @brief The name's pretty self explanatory, doncha think?
/// @details This is the remainder of non-inlined math functions that deal with initialization

#include "egolib/_math.h"
#include "egolib/TrigonometricTable.hpp"
#include "egolib/Math/Random.hpp"


//--------------------------------------------------------------------------------------------

// conversion functions


// limiting functions
    void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    int generate_irand_pair( const IPair num );
    int generate_irand_range( const FRange num );

// matrix functions


//--------------------------------------------------------------------------------------------
// CONVERSION FUNCTIONS
//--------------------------------------------------------------------------------------------

FACING_T vec_to_facing( const float dx, const float dy )
{
    return uint16_t(RadianToFacing(Ego::Math::Radians(std::atan2(dy, dx) + Ego::Math::pi<float>())));
}

//--------------------------------------------------------------------------------------------
void facing_to_vec( const Facing& facing, float * dx, float * dy )
{
    TLT::Index turn = TLT::get().fromFacing(FACING_T(facing - Facing(0x8000)));

    if ( NULL != dx )
    {
        *dx = TLT::get().cos(turn);
    }

    if ( NULL != dy )
    {
        *dy = TLT::get().sin(turn);
    }
}

//--------------------------------------------------------------------------------------------
// ROTATION FUNCTIONS
//--------------------------------------------------------------------------------------------
int terp_dir( const FACING_T& majordir, const FACING_T& minordir, const int weight )
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
void getadd_flt( const float min, const float value, const float max, float* valuetoadd )
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
int generate_irand_pair( const IPair num )
{
    /// @author ZZ
    /// @details This function generates a random number

    int tmp;
    int irand = Random::next(std::numeric_limits<uint16_t>::max());

    tmp = num.base;
    if ( num.rand > 1 )
    {
        tmp += irand % num.rand;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
int generate_irand_range( const FRange num )
{
    /// @author ZZ
    /// @details This function generates a random number

    IPair loc_pair;

    range_to_pair( num, &loc_pair );

    return generate_irand_pair( loc_pair );
}
