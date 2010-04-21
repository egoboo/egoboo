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

#include "particle.h"

#include "char.inl"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATION
//--------------------------------------------------------------------------------------------

INLINE PIP_REF  prt_get_ipip( const PRT_REF by_reference particle );
INLINE pip_t  * prt_get_ppip( const PRT_REF by_reference particle );
INLINE CHR_REF  prt_get_iowner( const PRT_REF by_reference iprt, int depth );
INLINE bool_t   prt_set_size( prt_t *, int size );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
INLINE PIP_REF prt_get_ipip( const PRT_REF by_reference iprt )
{
    prt_t * pprt;

    if ( !DEFINED_PRT( iprt ) ) return ( PIP_REF )MAX_PIP;
    pprt = PrtList.lst + iprt;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return ( PIP_REF )MAX_PIP;

    return pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
INLINE pip_t * prt_get_ppip( const PRT_REF by_reference iprt )
{
    prt_t * pprt;

    if ( !DEFINED_PRT( iprt ) ) return NULL;
    pprt = PrtList.lst + iprt;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return NULL;

    return PipStack.lst + pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t prt_set_size( prt_t * pprt, int size )
{
    float real_size;

    if ( !DEFINED_PPRT( pprt ) ) return bfalse;

    pprt->size = size;
    real_size  = size / 256.0;

    if ( 0 == pprt->size_stt )
    {
        // make the particle non-interacting if the initial size was 0
        pprt->bump.size = 0;
    }
    else if ( 0.0f == pprt->bump.size || 0.0f == size )
    {
        // just set the size, assuming a spherical particle
        pprt->bump.size     = real_size;
        pprt->bump.size_big = real_size * SQRT_TWO;
        pprt->bump.height   = real_size;
    }
    else
    {
        float mag = real_size / pprt->bump.size;

        // resize all dimensions equally
        pprt->bump.size     *= mag;
        pprt->bump.size_big *= mag;
        pprt->bump.height   *= mag;
    }

    bumper_to_oct_bb_0( pprt->bump, &( pprt->chr_prt_cv ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF prt_get_iowner( const PRT_REF by_reference iprt, int depth )
{
    /// BB@> A helper function for determining the owner of a paricle
    ///
    ///      @details There could be a possibility that a particle exists that was spawned by
    ///      another particle, but has lost contact with its original spawner. For instance
    ///      If an explosion particle bounces off of something with MISSILE_DEFLECT or
    ///      MISSILE_REFLECT, which subsequently dies before the particle...
    ///
    ///      That is actually pretty far fetched, but at some point it might make sense to
    ///      spawn particles just keeping track of the spawner (whether particle or character)
    ///      and working backward to any potential owner using this function. ;)
    ///
    ///      @note this function should be completely trivial for anything other than
    ///       namage particles created by an explosion

    CHR_REF iowner = ( CHR_REF )MAX_CHR;

    prt_t * pprt;

    // be careful because this can be recursive
    if ( depth > ( int )maxparticles - ( int )PrtList.free_count ) return ( CHR_REF )MAX_CHR;

    if ( !DEFINED_PRT( iprt ) ) return ( CHR_REF )MAX_CHR;
    pprt = PrtList.lst + iprt;

    if ( INGAME_CHR( pprt->owner_ref ) )
    {
        iowner = pprt->owner_ref;
    }
    else
    {
        // make a check for a stupid looping structure...
        // cannot be sure you could never get a loop, though

        if ( !ALLOCATED_PRT( pprt->parent_ref ) )
        {
            // make sure that a non valid parent_ref is marked as non-valid
            pprt->parent_ref = TOTAL_MAX_PRT;
            pprt->parent_guid = 0xFFFFFFFF;
        }
        else
        {
            // if a particle has been poofed, and another particle lives at that address,
            // it is possible that the pprt->parent_ref points to a valid particle that is
            // not the parent. Depending on how scrambled the list gets, there could actually
            // be looping structures. I have actually seen this, so don't laugh :)

            if ( PrtList.lst[pprt->parent_ref].obj_base.guid == pprt->parent_guid )
            {
                if ( iprt != pprt->parent_ref )
                {
                    iowner = prt_get_iowner( pprt->parent_ref, depth + 1 );
                }
            }
            else
            {
                // the parent particle doesn't exist anymore
                // fix the reference
                pprt->parent_ref = TOTAL_MAX_PRT;
                pprt->parent_guid = 0xFFFFFFFF;
            }
        }
    }

    return iowner;
}