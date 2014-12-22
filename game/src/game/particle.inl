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

#pragma once

#include "game/particle.h"
#include "egolib/_math.inl"
#include "egolib/bbox.inl"
#include "game/PrtList.inl"
#include "game/char.inl"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATION
//--------------------------------------------------------------------------------------------

static INLINE PIP_REF  prt_get_ipip( const PRT_REF particle );
static INLINE pip_t  * prt_get_ppip( const PRT_REF particle );
static INLINE CHR_REF  prt_get_iowner( const PRT_REF iprt, int depth );
static INLINE bool   prt_set_size( prt_t *, int size );
static INLINE float    prt_get_scale( prt_t * pprt );

static INLINE prt_bundle_t * prt_bundle_ctor( prt_bundle_t * pbundle );
static INLINE prt_bundle_t * prt_bundle_validate( prt_bundle_t * pbundle );
static INLINE prt_bundle_t * prt_bundle_set( prt_bundle_t * pbundle, prt_t * pprt );

static INLINE const float *prt_get_pos_v_const( const prt_t * pprt );
static INLINE float       *prt_get_pos_v( prt_t * pprt );
static INLINE bool     prt_get_pos( const prt_t * pprt, fvec3_base_t pos );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
static INLINE PIP_REF prt_get_ipip( const PRT_REF iprt )
{
    prt_t * pprt;

    if ( !DEFINED_PRT( iprt ) ) return INVALID_PIP_REF;
    pprt = PrtList_get_ptr( iprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return INVALID_PIP_REF;

    return pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
static INLINE pip_t * prt_get_ppip( const PRT_REF iprt )
{
    prt_t * pprt;

    if ( !DEFINED_PRT( iprt ) ) return NULL;
    pprt = PrtList_get_ptr( iprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return NULL;

    return PipStack_get_ptr( pprt->pip_ref );
}

//--------------------------------------------------------------------------------------------
static INLINE bool prt_set_size( prt_t * pprt, int size )
{
    pip_t *ppip;

    if ( !DEFINED_PPRT( pprt ) ) return false;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return false;
    ppip = PipStack_get_ptr( pprt->pip_ref );

    // set the graphical size
    pprt->size = size;

    // set the bumper size, if available
    if ( 0 == pprt->bump_size_stt )
    {
        // make the particle non-interacting if the initial bumper size was 0
        pprt->bump_real.size   = 0;
        pprt->bump_padded.size = 0;
    }
    else
    {
        float real_size  = FP8_TO_FLOAT( size ) * prt_get_scale( pprt );

        if ( 0.0f == pprt->bump_real.size || 0.0f == size )
        {
            // just set the size, assuming a spherical particle
            pprt->bump_real.size     = real_size;
            pprt->bump_real.size_big = real_size * SQRT_TWO;
            pprt->bump_real.height   = real_size;
        }
        else
        {
            float mag = real_size / pprt->bump_real.size;

            // resize all dimensions equally
            pprt->bump_real.size     *= mag;
            pprt->bump_real.size_big *= mag;
            pprt->bump_real.height   *= mag;
        }

        // make sure that the virtual bumper size is at least as big as what is in the pip file
        pprt->bump_padded.size     = std::max( pprt->bump_real.size,     ((float)ppip->bump_size) );
        pprt->bump_padded.size_big = std::max( pprt->bump_real.size_big, ((float)ppip->bump_size) * ((float)SQRT_TWO) );
        pprt->bump_padded.height   = std::max( pprt->bump_real.height,   ((float)ppip->bump_height) );
    }

    // set the real size of the particle
    oct_bb_set_bumper( &( pprt->prt_min_cv ), pprt->bump_real );

    // use the padded bumper to figure out the chr_max_cv
    oct_bb_set_bumper( &( pprt->prt_max_cv ), pprt->bump_padded );

    return true;
}

//--------------------------------------------------------------------------------------------
static INLINE CHR_REF prt_get_iowner( const PRT_REF iprt, int depth )
{
    /// @author BB
    /// @details A helper function for determining the owner of a paricle
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
    /// @note this function should be completely trivial for anything other than
    ///       damage particles created by an explosion

    CHR_REF iowner = INVALID_CHR_REF;

    prt_t * pprt;

    // be careful because this can be recursive
    if ( depth > ( int )maxparticles - ( int )PrtList.free_count ) return INVALID_CHR_REF;

    if ( !DEFINED_PRT( iprt ) ) return INVALID_CHR_REF;
    pprt = PrtList_get_ptr( iprt );

    if ( DEFINED_CHR( pprt->owner_ref ) )
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
            pprt->parent_ref = INVALID_PRT_REF;
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
                pprt->parent_ref = INVALID_PRT_REF;
                pprt->parent_guid = 0xFFFFFFFF;
            }
        }
    }

    return iowner;
}

//--------------------------------------------------------------------------------------------
static INLINE float prt_get_scale( prt_t * pprt )
{
    /// @author BB
    /// @details get the scale factor between the "graphical size" of the particle and the actual
    ///               display size

    float scale = 0.25f;

    if ( !DEFINED_PPRT( pprt ) ) return scale;

    // set some particle dependent properties
    switch ( pprt->type )
    {
        case SPRITE_SOLID: scale *= 0.9384f; break;
        case SPRITE_ALPHA: scale *= 0.9353f; break;
        case SPRITE_LIGHT: scale *= 1.5912f; break;
    }

    return scale;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE prt_bundle_t * prt_bundle_ctor( prt_bundle_t * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    pbundle->prt_ref = INVALID_PRT_REF;
    pbundle->prt_ptr = NULL;

    pbundle->pip_ref = INVALID_PIP_REF;
    pbundle->pip_ptr = NULL;

    return pbundle;
}

//--------------------------------------------------------------------------------------------
static INLINE prt_bundle_t * prt_bundle_validate( prt_bundle_t * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    if ( ALLOCATED_PRT( pbundle->prt_ref ) )
    {
        pbundle->prt_ptr = PrtList_get_ptr( pbundle->prt_ref );
    }
    else if ( NULL != pbundle->prt_ptr )
    {
        pbundle->prt_ref = GET_REF_PPRT( pbundle->prt_ptr );
    }
    else
    {
        pbundle->prt_ref = INVALID_PRT_REF;
        pbundle->prt_ptr = NULL;
    }

    if ( !LOADED_PIP( pbundle->pip_ref ) && NULL != pbundle->prt_ptr )
    {
        pbundle->pip_ref = pbundle->prt_ptr->pip_ref;
    }

    if ( LOADED_PIP( pbundle->pip_ref ) )
    {
        pbundle->pip_ptr = PipStack_get_ptr( pbundle->pip_ref );
    }
    else
    {
        pbundle->pip_ref = INVALID_PIP_REF;
        pbundle->pip_ptr = NULL;
    }

    return pbundle;
}

//--------------------------------------------------------------------------------------------
static INLINE prt_bundle_t * prt_bundle_set( prt_bundle_t * pbundle, prt_t * pprt )
{
    if ( NULL == pbundle ) return NULL;

    // blank out old data
    pbundle = prt_bundle_ctor( pbundle );

    if ( NULL == pbundle || NULL == pprt ) return pbundle;

    // set the particle pointer
    pbundle->prt_ptr = pprt;

    // validate the particle data
    pbundle = prt_bundle_validate( pbundle );

    return pbundle;
}

//--------------------------------------------------------------------------------------------
bool prt_get_pos( const prt_t * pprt, fvec3_base_t pos )
{
    float * copy_rv;

    if ( !ALLOCATED_PPRT( pprt ) ) return false;

    copy_rv = fvec3_base_copy( pos, pprt->pos.v );

    return ( NULL == copy_rv ) ? false : true;
}

//--------------------------------------------------------------------------------------------
static INLINE const float * prt_get_pos_v_const( const prt_t * pprt )
{
    static fvec3_t vtmp = ZERO_VECT3;

    if ( !ALLOCATED_PPRT( pprt ) ) return vtmp.v;

    return pprt->pos.v;
}

//--------------------------------------------------------------------------------------------
static INLINE float * prt_get_pos_v( prt_t * pprt )
{
    static fvec3_t vtmp = ZERO_VECT3;

    if ( !ALLOCATED_PPRT( pprt ) ) return vtmp.v;

    return pprt->pos.v;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _particle_inl
