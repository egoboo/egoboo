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

/// @file egoboo_math.c
/// @brief The name's pretty self explanatory, doncha think?
/// @details This is the remainder of non-inlined math functions that deal with initialization

#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float turntosin[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to sine
float turntocos[TRIG_TABLE_SIZE];           // Convert chrturn>>2...  to cosine

Uint32  randindex = 0;
Uint16  randie[RANDIE_COUNT];

//--------------------------------------------------------------------------------------------
void make_turntosin( void )
{
    /// @details ZZ@> This function makes the lookup table for chrturn...
    int cnt;
    float ftmp = TWO_PI / ( float )TRIG_TABLE_SIZE;

    for ( cnt = 0; cnt < TRIG_TABLE_SIZE; cnt++ )
    {
        turntosin[cnt] = SIN( cnt * ftmp );
        turntocos[cnt] = COS( cnt * ftmp );
    }
}

//--------------------------------------------------------------------------------------------
void make_randie()
{
    /// @details ZZ@> This function makes the random number table
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
