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



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float turntosin[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to sine
float turntocos[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to cosine

Uint32  randindex = 0;
Uint16  randie[RANDIE_COUNT];

//--------------------------------------------------------------------------------------------
void make_turntosin( void )
{
    /// @author ZZ
    /// @details This function makes the lookup table for chrturn...

    int cnt;
    const float ftmp = TWO_PI / ( float )TRIG_TABLE_SIZE;

    for ( cnt = 0; cnt < TRIG_TABLE_SIZE; cnt++ )
    {
        turntosin[cnt] = SIN( cnt * ftmp );
        turntocos[cnt] = COS( cnt * ftmp );
    }
}

//--------------------------------------------------------------------------------------------
void make_randie( void )
{
    /// @author ZZ
    /// @details This function makes the random number table
    int tnc, cnt;

    // Fill in the basic values
    for ( cnt = 0; cnt < RANDIE_COUNT; cnt++ )
    {
        randie[cnt] = 0;
    }

    // Keep adjusting those values
    for ( tnc = 0; tnc < 20; tnc++ )
    {
        for ( cnt = 0; cnt < RANDIE_COUNT; cnt++ )
        {
            randie[cnt] = ( randie[cnt] << 1 ) + rand();
        }
    }

    // All done
    randindex = 0;
}


// conversion functions
    FACING_T vec_to_facing( const float dx, const float dy );
    void     facing_to_vec( const FACING_T facing, float * dx, float * dy );

// rotation functions
    int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight );

// limiting functions
    void getadd_int( const int min, const int value, const int max, int* valuetoadd );
    void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    int generate_irand_pair( const IPair num );
    int generate_irand_range( const FRange num );
    int generate_randmask( const int base, const Uint32 mask );

// matrix functions


//--------------------------------------------------------------------------------------------
// CONVERSION FUNCTIONS
//--------------------------------------------------------------------------------------------

FACING_T vec_to_facing( const float dx, const float dy )
{
    return ( FACING_T )( RAD_TO_FACING( ATAN2( dy, dx ) + PI ) );
}

//--------------------------------------------------------------------------------------------
void facing_to_vec( const FACING_T facing, float * dx, float * dy )
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
int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight )
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
void getadd_int( const int min, const int value, const int max, int* valuetoadd )
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
    int irand = RANDIE;

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

//--------------------------------------------------------------------------------------------
int generate_randmask( const int base, const Uint32 mask )
{
    /// @author ZZ
    /// @details This function generates a random number

    int tmp;
    int irand = RANDIE;

    tmp = base;
    tmp += irand & mask;

    return tmp;
}
