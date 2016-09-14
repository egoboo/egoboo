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

#include "egolib/Log/_Include.hpp"
#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
void pair_to_range( IPair pair, Ego::Math::Interval<float> * prange )
{
    /// @author ZZ
    /// @details convert from a pair to a range

    if (pair.base < 0) {
        Log::get().warn("We got a randomization error again! (Base is less than 0)\n");
    }

    if (pair.rand < 0) {
        Log::get().warn("We got a randomization error again! (rand is less than 0)\n");
    }

    if (nullptr != prange) {
        float fFrom = FP8_TO_FLOAT(pair.base);
        float fTo = FP8_TO_FLOAT(pair.base + pair.rand);
        *prange = Ego::Math::Interval<float>(std::min(fFrom, fTo), std::max(fFrom, fTo));
    }
}

//--------------------------------------------------------------------------------------------
void range_to_pair(Ego::Math::Interval<float> range, IPair * ppair )
{
    /// @author ZZ
    /// @details convert from a range to a pair

    // @todo Remove this check, not possible with the new API.
    if ( range.getLowerbound() > range.getUpperbound() )
    {
		Log::get().warn( "We got a range error! (to is less than from)\n" );
    }

    if ( NULL != ppair )
    {
        float fFrom = range.getLowerbound();
        float fTo   = range.getUpperbound();

        ppair->base = FLOAT_TO_FP8( fFrom );
        ppair->rand = FLOAT_TO_FP8( fTo - fFrom );
    }
}

//--------------------------------------------------------------------------------------------
void ints_to_range( int ibase, int irand, Ego::Math::Interval<float> * prange )
{
    IPair pair_tmp;

    pair_tmp.base = ibase;
    pair_tmp.rand = irand;

    pair_to_range( pair_tmp, prange );
}

//--------------------------------------------------------------------------------------------
void floats_to_pair( float vmin, float vmax, IPair * ppair )
{
    Ego::Math::Interval<float> range_tmp(vmin, vmax);
    range_to_pair( range_tmp, ppair );
}
