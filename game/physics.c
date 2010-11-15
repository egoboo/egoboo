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

/// @file physics.c

#include "physics.inl"

#include "game.h"

#include "char.inl"
#include "particle.inl"
#include "mesh.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float   hillslide       =  1.00f;
float   slippyfriction  =  1.00f;
float   airfriction     =  0.91f;
float   waterfriction   =  0.80f;
float   noslipfriction  =  0.91f;
float   gravity         = -1.00f;
float   platstick       =  0.01f;
fvec3_t windspeed       = ZERO_VECT3;
fvec3_t waterspeed      = ZERO_VECT3;

static int breadcrumb_guid = 0;

const float air_friction = 0.9868f;
const float ice_friction = 0.9738f;  // the square of air_friction

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t phys_estimate_chr_chr_normal( oct_vec_t opos_a, oct_vec_t opos_b, oct_vec_t odepth, float exponent, fvec3_base_t nrm )
{
    bool_t retval;

    // is everything valid?
    if ( NULL == opos_a || NULL == opos_b || NULL == odepth || NULL == nrm ) return bfalse;

    // initialize the vector
    nrm[kX] = nrm[kY] = nrm[kZ] = 0.0f;

    if ( odepth[OCT_X] <= 0.0f )
    {
        odepth[OCT_X] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_X] - opos_a[OCT_X];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kX] += sgn / POW( odepth[OCT_X] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_Y] <= 0.0f )
    {
        odepth[OCT_Y] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_Y] - opos_a[OCT_Y];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kY] += sgn / POW( odepth[OCT_Y] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_XY] <= 0.0f )
    {
        odepth[OCT_XY] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_XY] - opos_a[OCT_XY];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kX] += sgn / POW( odepth[OCT_XY] / PLATTOLERANCE, exponent );
        nrm[kY] += sgn / POW( odepth[OCT_XY] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_YX] <= 0.0f )
    {
        odepth[OCT_YX] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_YX] - opos_a[OCT_YX];
        sgn = sgn > 0 ? -1 : 1;
        nrm[kX] -= sgn / POW( odepth[OCT_YX] / PLATTOLERANCE, exponent );
        nrm[kY] += sgn / POW( odepth[OCT_YX] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_Z] <= 0.0f )
    {
        odepth[OCT_Z] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_Z] - opos_a[OCT_Z];

        sgn = sgn > 0 ? -1 : 1;

        nrm[kZ] += sgn / POW( exponent * odepth[OCT_Z] / PLATTOLERANCE, exponent );
    }

    retval = bfalse;
    if ( ABS( nrm[kX] ) + ABS( nrm[kY] ) + ABS( nrm[kZ] ) > 0.0f )
    {
        fvec3_t vtmp = fvec3_normalize( nrm );
        memcpy( nrm, vtmp.v, sizeof( fvec3_base_t ) );
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv oct_bb_intersect_index( int index, oct_bb_t src1, oct_vec_t opos1, oct_vec_t ovel1, oct_bb_t src2, oct_vec_t opos2, oct_vec_t ovel2, int test_platform, float *tmin, float *tmax )
{
    float tolerance1, tolerance2;
    float vdiff;
    float src1_min, src1_max;
    float src2_min, src2_max;

    if ( NULL == tmin || NULL == tmax ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    vdiff = ovel2[index] - ovel1[index];
    if ( vdiff == 0.0f ) return rv_fail;

    tolerance1 = (( OCT_Z == index ) && ( test_platform & PHYS_PLATFORM_OBJ1 ) ) ? PLATTOLERANCE : 0.0f;
    tolerance2 = (( OCT_Z == index ) && ( test_platform & PHYS_PLATFORM_OBJ2 ) ) ? PLATTOLERANCE : 0.0f;

    src1_min = src1.mins[index] + opos1[index];
    src1_max = src1.maxs[index] + opos1[index];
    src2_min = src2.mins[index] + opos2[index];
    src2_max = src2.maxs[index] + opos2[index];

    if ( 0.0f == tolerance1 && 0.0f == tolerance2 )
    {
        // NEITHER ia a platform

        float time[4];

        time[0] = ( src1_min - src2_min ) / vdiff;
        time[1] = ( src1_min - src2_max ) / vdiff;
        time[2] = ( src1_max - src2_min ) / vdiff;
        time[3] = ( src1_max - src2_max ) / vdiff;

        *tmin = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        *tmax = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );
    }
    else if ( 0.0f == tolerance1 )
    {
        // ONLY src2 is a platform
        float time[4];

        time[0] = ( src1_min - ( src2_min - tolerance2 ) ) / vdiff;
        time[1] = ( src1_min - ( src2_max + tolerance2 ) ) / vdiff;
        time[2] = ( src1_max - ( src2_min - tolerance2 ) ) / vdiff;
        time[3] = ( src1_max - ( src2_max + tolerance2 ) ) / vdiff;

        *tmin = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        *tmax = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );
    }
    else if ( 0.0f == tolerance2 )
    {
        // ONLY src1 is a platform
        float time[4];

        time[0] = (( src1_min - tolerance1 ) - src2_min ) / vdiff;
        time[1] = (( src1_min - tolerance1 ) - src2_max ) / vdiff;
        time[2] = (( src1_max + tolerance1 ) - src2_min ) / vdiff;
        time[3] = (( src1_max + tolerance1 ) - src2_max ) / vdiff;

        *tmin = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        *tmax = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );
    }

    else if ( tolerance1 > 0.0f && tolerance2 > 0.0f )
    {
        // BOTH are platforms
        // they cannot both act as plaforms at the same time, so do 8 tests

        float time[8];
        float tmp_min1, tmp_max1;
        float tmp_min2, tmp_max2;

        time[0] = ( src1_min - ( src2_min - tolerance2 ) ) / vdiff;
        time[1] = ( src1_min - ( src2_max + tolerance2 ) ) / vdiff;
        time[2] = ( src1_max - ( src2_min - tolerance2 ) ) / vdiff;
        time[3] = ( src1_max - ( src2_max + tolerance2 ) ) / vdiff;
        tmp_min1 = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        tmp_max1 = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );

        time[4] = (( src1_min - tolerance1 ) - src2_min ) / vdiff;
        time[5] = (( src1_min - tolerance1 ) - src2_max ) / vdiff;
        time[6] = (( src1_max + tolerance1 ) - src2_min ) / vdiff;
        time[7] = (( src1_max + tolerance1 ) - src2_max ) / vdiff;
        tmp_min2 = MIN( MIN( time[4], time[5] ), MIN( time[6], time[7] ) );
        tmp_max2 = MAX( MAX( time[4], time[5] ), MAX( time[6], time[7] ) );

        *tmin = MIN( tmp_min1, tmp_min2 );
        *tmax = MAX( tmp_max1, tmp_max2 );
    }

    else
    {
        // this should literally be impossible to reach
        EGOBOO_ASSERT( bfalse );
    }

    // normalize the results for the diagonal directions
    if ( OCT_XY == index || OCT_YX == index )
    {
        *tmin *= INV_SQRT_TWO;
        *tmax *= INV_SQRT_TWO;
    }

    // cannot possibly have more history than this
    *tmin = MAX( *tmin, -(( signed )( update_wld + 1 ) ) );

    if ( *tmax <= *tmin ) return rv_fail;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv oct_bb_intersect_close_index( int index, oct_bb_t src1, oct_vec_t opos1, oct_vec_t ovel1, oct_bb_t src2, oct_vec_t opos2, oct_vec_t ovel2, int test_platform, float *tmin, float *tmax )
{
    float tolerance1, tolerance2;
    float diff;

    if ( NULL == tmin || NULL == tmax ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    // in the z-direction you always have to deal with the bounding box size
    if ( OCT_Z == index ) return oct_bb_intersect_index( index, src1, opos1, ovel1, src2, opos2, ovel2, test_platform, tmin, tmax );

    diff = ovel2[index] - ovel1[index];
    if ( diff == 0.0f ) return rv_fail;

    tolerance1 = (( OCT_Z == index ) && ( test_platform & PHYS_PLATFORM_OBJ1 ) ) ? PLATTOLERANCE : 0.0f;
    tolerance2 = (( OCT_Z == index ) && ( test_platform & PHYS_PLATFORM_OBJ2 ) ) ? PLATTOLERANCE : 0.0f;

    if ( 0.0f == tolerance1 && 0.0f == tolerance2 )
    {
        float time[8];
        float tmp_min1, tmp_max1;
        float tmp_min2, tmp_max2;

        time[0] = ( opos1[index] - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[1] = ( opos1[index] - ( src2.maxs[index] + opos2[index] ) ) / diff;
        time[2] = ( opos1[index] - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[3] = ( opos1[index] - ( src2.maxs[index] + opos2[index] ) ) / diff;
        tmp_min1 = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        tmp_max1 = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );

        time[4] = (( src1.mins[index] + opos1[index] ) - opos2[index] ) / diff;
        time[5] = (( src1.mins[index] + opos1[index] ) - opos2[index] ) / diff;
        time[6] = (( src1.maxs[index] + opos1[index] ) - opos2[index] ) / diff;
        time[7] = (( src1.maxs[index] + opos1[index] ) - opos2[index] ) / diff;
        tmp_min2 = MIN( MIN( time[4], time[5] ), MIN( time[6], time[7] ) );
        tmp_max2 = MAX( MAX( time[4], time[5] ), MAX( time[6], time[7] ) );

        *tmin = MIN( tmp_min1, tmp_min2 );
        *tmax = MAX( tmp_max1, tmp_max2 );
    }
    else
    {
        float time[8];
        float tmp_min1, tmp_max1;
        float tmp_min2, tmp_max2;

        time[0] = ( opos1[index] - ( src2.mins[index] - tolerance2 + opos2[index] ) ) / diff;
        time[1] = ( opos1[index] - ( src2.maxs[index] + tolerance2 + opos2[index] ) ) / diff;
        time[2] = ( opos1[index] - ( src2.mins[index] - tolerance2 + opos2[index] ) ) / diff;
        time[3] = ( opos1[index] - ( src2.maxs[index] + tolerance2 + opos2[index] ) ) / diff;
        tmp_min1 = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        tmp_max1 = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );

        time[4] = (( src1.mins[index] - tolerance1 + opos1[index] ) - opos2[index] ) / diff;
        time[5] = (( src1.mins[index] - tolerance1 + opos1[index] ) - opos2[index] ) / diff;
        time[6] = (( src1.maxs[index] + tolerance1 + opos1[index] ) - opos2[index] ) / diff;
        time[7] = (( src1.maxs[index] + tolerance1 + opos1[index] ) - opos2[index] ) / diff;
        tmp_min2 = MIN( MIN( time[4], time[5] ), MIN( time[6], time[7] ) );
        tmp_max2 = MAX( MAX( time[4], time[5] ), MAX( time[6], time[7] ) );

        *tmin = MIN( tmp_min1, tmp_min2 );
        *tmax = MAX( tmp_max1, tmp_max2 );
    }

    // normalize the results for the diagonal directions
    if ( OCT_XY == index || OCT_YX == index )
    {
        *tmin *= INV_SQRT_TWO;
        *tmax *= INV_SQRT_TWO;
    }

    if ( *tmax < *tmin ) return rv_fail;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool_t phys_intersect_oct_bb( const oct_bb_t src1_orig, const fvec3_base_t pos1, const fvec3_base_t vel1, const oct_bb_t src2_orig, const fvec3_base_t pos2, const fvec3_base_t vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax )
{
    /// @details BB@> A test to determine whether two "fast moving" objects are interacting within a frame.
    ///               Designed to determine whether a bullet particle will interact with character.

    oct_bb_t src1, src2;
    oct_bb_t exp1, exp2;

    oct_vec_t opos1, opos2;
    oct_vec_t ovel1, ovel2;

    int    cnt, index;
    bool_t found;
    float  tolerance;
    float  local_tmin, local_tmax;
    float  tmp_min, tmp_max;

    int    failure_count = 0;
    bool_t failure[OCT_COUNT];

    // handle optional parameters
    if ( NULL == tmin ) tmin = &local_tmin;
    if ( NULL == tmax ) tmax = &local_tmax;

    //// do the objects interact at the very beginning of the update?
    //if ( test_interaction_2( src1_orig, pos2, src2_orig, pos2, test_platform ) )
    //{
    //    oct_bb_t tmp1, tmp2;

    //    oct_bb_add_vector( src1_orig, pos1, &tmp1 );
    //    oct_bb_add_vector( src2_orig, pos2, &tmp2 );

    //    oct_bb_intersection( &tmp1, &tmp2, pdst );

    //    *tmin = *tmax = -1.0f;

    //    return btrue;
    //}

    // convert the position and velocity vectors to octagonal format
    oct_vec_ctor( ovel1, vel1 );
    oct_vec_ctor( opos1, pos1 );

    oct_vec_ctor( ovel2, vel2 );
    oct_vec_ctor( opos2, pos2 );

    // cycle through the coordinates to see when the two volumes might coincide
    found = bfalse;
    *tmin = *tmax = -1.0f;
    if ( 0 == fvec3_dist_abs( vel1, vel2 ) )
    {
        // no relative motion, so avoid the loop to save time
        failure_count = OCT_COUNT;
    }
    else
    {
        for ( index = 0; index < OCT_COUNT; index++ )
        {
            egoboo_rv retval;

            retval = oct_bb_intersect_index( index, src1_orig, opos1, ovel1, src2_orig, opos2, ovel2, test_platform, &tmp_min, &tmp_max );
            if ( rv_fail == retval )
            {
                // This case will only occur if the objects are not moving relative to each other.

                failure[index] = btrue;
                failure_count++;
            }
            else if ( rv_success == retval )
            {
                failure[index] = bfalse;

                if ( !found )
                {
                    *tmin = tmp_min;
                    *tmax = tmp_max;
                    found = btrue;
                }
                else
                {
                    *tmin = MAX( *tmin, tmp_min );
                    *tmax = MIN( *tmax, tmp_max );
                }

                if ( *tmax < *tmin ) return bfalse;
            }
        }
    }

    if ( OCT_COUNT == failure_count )
    {
        // No relative motion on any axis.
        // Just say that they are interacting for the whole frame

        *tmin = 0.0f;
        *tmax = 1.0f;
    }
    else
    {
        // some relative motion found

        // if the objects do not interact this frame let the caller know
        if ( *tmin > 1.0f || *tmax < 0.0f ) return bfalse;

        // limit the negative values of time to the start of the module
        if (( *tmin ) + update_wld < 0 ) *tmin = -(( Sint32 )update_wld );
    }

    // clip the interaction time to just one frame
    tmp_min = CLIP( *tmin, 0.0f, 1.0f );
    tmp_max = CLIP( *tmax, 0.0f, 1.0f );

    // shift the source bounding boxes to be centered on the given positions
    oct_bb_add_vector( src1_orig, pos1, &src1 );
    oct_bb_add_vector( src2_orig, pos2, &src2 );

    // determine the expanded collision volumes for both objects
    phys_expand_oct_bb( src1, vel1, tmp_min, tmp_max, &exp1 );
    phys_expand_oct_bb( src2, vel2, tmp_min, tmp_max, &exp2 );

    // determine the intersection of these two volumes
    oct_bb_intersection( &exp1, &exp2, pdst );

    // check to see if there is any possibility of interaction at all
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        if ( pdst->mins[cnt] > pdst->maxs[cnt] ) return bfalse;
    }

    tolerance = ( 0 == test_platform ) ? 0.0f : PLATTOLERANCE;
    if ( pdst->mins[OCT_Z] > pdst->maxs[OCT_Z] + tolerance ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t oct_bb_intersect_close( oct_bb_t src1, fvec3_t pos1, fvec3_t vel1, oct_bb_t src2, fvec3_t pos2, fvec3_t vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax )
{
    /// @details BB@> A test to determine whether two "fast moving" objects are interacting within a frame.
    ///               Designed to determine whether a bullet particle will interact with character.

    oct_bb_t exp1, exp2;
    oct_bb_t intersection;

    oct_vec_t opos1, opos2;
    oct_vec_t ovel1, ovel2;

    int    cnt, index;
    bool_t found;
    float  tolerance;
    float  local_tmin, local_tmax;

    // handle optional parameters
    if ( NULL == tmin ) tmin = &local_tmin;
    if ( NULL == tmax ) tmax = &local_tmax;

    // do the objects interact at the very beginning of the update?
    if ( test_interaction_2( src1, pos2, src2, pos2, test_platform ) )
    {
        if ( NULL != pdst )
        {
            oct_bb_intersection( &src1, &src2, pdst );
        }

        return btrue;
    }

    // convert the position and velocity vectors to octagonal format
    oct_vec_ctor( ovel1, vel1.v );
    oct_vec_ctor( opos1, pos1.v );

    oct_vec_ctor( ovel2, vel2.v );
    oct_vec_ctor( opos2, pos2.v );

    // cycle through the coordinates to see when the two volumes might coincide
    found = bfalse;
    *tmin = *tmax = -1.0f;
    for ( index = 0; index < OCT_COUNT; index ++ )
    {
        egoboo_rv retval;
        float tmp_min, tmp_max;

        retval = oct_bb_intersect_close_index( index, src1, opos1, ovel1, src2, opos2, ovel2, test_platform, &tmp_min, &tmp_max );
        if ( rv_fail == retval ) return bfalse;

        if ( rv_success == retval )
        {
            if ( !found )
            {
                *tmin = tmp_min;
                *tmax = tmp_max;
                found = btrue;
            }
            else
            {
                *tmin = MAX( *tmin, tmp_min );
                *tmax = MIN( *tmax, tmp_max );
            }
        }

        if ( *tmax < *tmin ) return bfalse;
    }

    // if the objects do not interact this frame let the caller know
    if ( *tmin > 1.0f || *tmax < 0.0f ) return bfalse;

    // determine the expanded collision volumes for both objects
    phys_expand_oct_bb( src1, vel1.v, *tmin, *tmax, &exp1 );
    phys_expand_oct_bb( src2, vel2.v, *tmin, *tmax, &exp2 );

    // determine the intersection of these two volumes
    oct_bb_intersection( &exp1, &exp2, &intersection );

    // check to see if there is any possibility of interaction at all
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        if ( intersection.mins[cnt] > intersection.maxs[cnt] ) return bfalse;
    }

    tolerance = ( 0 == test_platform ) ? 0.0f : PLATTOLERANCE;
    if ( intersection.mins[OCT_Z] > intersection.maxs[OCT_Z] + tolerance ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t phys_expand_oct_bb( const oct_bb_t src, const fvec3_base_t vel, const float tmin, const float tmax, oct_bb_t * pdst )
{
    /// @details BB@> use the velocity of an object and its oct_bb_t to determine the
    ///               amount of territory that an object will cover in the range [tmin,tmax].
    ///               One update equals [tmin,tmax] == [0,1].

    float abs_vel;
    oct_bb_t tmp_min, tmp_max;

    abs_vel = ABS( vel[kX] ) + ABS( vel[kY] ) + ABS( vel[kZ] );
    if ( 0.0f == abs_vel )
    {
        if ( NULL != pdst )
        {
            *pdst = src;
        }
        return btrue;
    }

    // determine the bounding volume at t == tmin
    if ( 0.0f == tmin )
    {
        tmp_min = src;
    }
    else
    {
        fvec3_t pos_min;

        pos_min.x = vel[kX] * tmin;
        pos_min.y = vel[kY] * tmin;
        pos_min.z = vel[kZ] * tmin;

        // adjust the bounding box to take in the position at the next step
        if ( !oct_bb_add_vector( src, pos_min.v, &tmp_min ) ) return bfalse;
    }

    // determine the bounding volume at t == tmax
    if ( tmax == 0.0f )
    {
        tmp_max = src;
    }
    else
    {
        fvec3_t pos_max;

        pos_max.x = vel[kX] * tmax;
        pos_max.y = vel[kY] * tmax;
        pos_max.z = vel[kZ] * tmax;

        // adjust the bounding box to take in the position at the next step
        if ( !oct_bb_add_vector( src, pos_max.v, &tmp_max ) ) return bfalse;
    }

    // determine bounding box for the range of times
    if ( !oct_bb_union( &tmp_min, &tmp_max, pdst ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t phys_expand_chr_bb( chr_t * pchr, float tmin, float tmax, oct_bb_t * pdst )
{
    /// @details BB@> use the object velocity to figure out where the volume that the character will
    ///               occupy during this update. Use the loser chr_max_cv and include extra height if
    ///               it is a platform.

    oct_bb_t tmp_oct1, tmp_oct2;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // copy the volume
    tmp_oct1 = pchr->chr_max_cv;

    if ( pchr->platform )
    {
        // expand the interaction range so that we will correctly detect interactions with platforms
        tmp_oct1.maxs[OCT_Z] += PLATTOLERANCE;
    }

    // add in the current position to the bounding volume
    oct_bb_add_vector( tmp_oct1, pchr->pos.v, &tmp_oct2 );

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb( tmp_oct2, pchr->vel.v, tmin, tmax, pdst );
}

//--------------------------------------------------------------------------------------------
bool_t phys_expand_prt_bb( prt_t * pprt, float tmin, float tmax, oct_bb_t * pdst )
{
    /// @details BB@> use the object velocity to figure out where the volume that the particle will
    ///               occupy during this update

    oct_bb_t tmp_oct;

    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // add in the current position to the bounding volume
    oct_bb_add_vector( pprt->prt_cv, prt_get_pos_v( pprt ), &tmp_oct );

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb( tmp_oct, pprt->vel.v, tmin, tmax, pdst );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_init_chr( breadcrumb_t * bc, chr_t * pchr )
{
    if ( NULL == bc ) return bc;

    memset( bc, 0, sizeof( breadcrumb_t ) );
    bc->time = update_wld;

    if ( NULL == pchr ) return bc;

    bc->bits   = pchr->stoppedby;
    bc->radius = pchr->bump_1.size;
    bc->pos.x  = ( floor( pchr->pos.x / GRID_SIZE ) + 0.5f ) * GRID_SIZE;
    bc->pos.y  = ( floor( pchr->pos.y / GRID_SIZE ) + 0.5f ) * GRID_SIZE;
    bc->pos.z  = pchr->pos.z;

    bc->grid   = mesh_get_grid( PMesh, bc->pos.x, bc->pos.y );
    bc->valid  = ( 0 == mesh_test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) );

    bc->id = breadcrumb_guid++;

    return bc;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_init_prt( breadcrumb_t * bc, prt_t * pprt )
{
    BIT_FIELD bits = 0;
    pip_t * ppip;

    if ( NULL == bc ) return bc;

    memset( bc, 0, sizeof( breadcrumb_t ) );
    bc->time = update_wld;

    if ( NULL == pprt ) return bc;

    ppip = prt_get_ppip( GET_REF_PPRT( pprt ) );
    if ( NULL == ppip ) return bc;

    bits = MPDFX_IMPASS;
    if ( 0 != ppip->bump_money ) SET_BIT( bits, MPDFX_WALL );

    bc->bits   = bits;
    bc->radius = pprt->bump_real.size;

    bc->pos = prt_get_pos( pprt );
    bc->pos.x  = ( floor( bc->pos.x / GRID_SIZE ) + 0.5f ) * GRID_SIZE;
    bc->pos.y  = ( floor( bc->pos.y / GRID_SIZE ) + 0.5f ) * GRID_SIZE;

    bc->grid   = mesh_get_grid( PMesh, bc->pos.x, bc->pos.y );
    bc->valid  = ( 0 == mesh_test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) );

    bc->id = breadcrumb_guid++;

    return bc;
}

//--------------------------------------------------------------------------------------------
int breadcrumb_cmp( const void * lhs, const void * rhs )
{
    // comparison to sort from oldest to newest
    int retval;

    breadcrumb_t * bc_lhs = ( breadcrumb_t * )lhs;
    breadcrumb_t * bc_rhs = ( breadcrumb_t * )rhs;

    retval = ( int )(( Sint64 )bc_rhs->time - ( Sint64 )bc_lhs->time );

    if ( 0 == retval )
    {
        retval = ( int )(( Sint64 )bc_rhs->id - ( Sint64 )bc_lhs->id );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t breadcrumb_list_full( breadcrumb_list_t *  lst )
{
    if ( NULL == lst || !lst->on ) return btrue;

    lst->count = CLIP( lst->count, 0, MAX_BREADCRUMB );

    return ( lst->count >= MAX_BREADCRUMB );
}

//--------------------------------------------------------------------------------------------
bool_t breadcrumb_list_empty( breadcrumb_list_t * lst )
{
    if ( NULL == lst || !lst->on ) return btrue;

    lst->count = CLIP( lst->count, 0, MAX_BREADCRUMB );

    return ( 0 == lst->count );
}

//--------------------------------------------------------------------------------------------
void breadcrumb_list_compact( breadcrumb_list_t * lst )
{
    int cnt, tnc;

    if ( NULL == lst || !lst->on ) return;

    // compact the list of breadcrumbs
    for ( cnt = 0, tnc = 0; cnt < lst->count; cnt ++ )
    {
        breadcrumb_t * bc_src = lst->lst + cnt;
        breadcrumb_t * bc_dst = lst->lst + tnc;

        if ( bc_src->valid )
        {
            if ( bc_src != bc_dst )
            {
                memcpy( bc_dst, bc_src, sizeof( breadcrumb_t ) );
            }

            tnc++;
        }
    }
    lst->count = tnc;
}

//--------------------------------------------------------------------------------------------
void breadcrumb_list_validate( breadcrumb_list_t * lst )
{
    int cnt, invalid_cnt;

    if ( NULL == lst || !lst->on ) return;

    // invalidate all bad breadcrumbs
    for ( cnt = 0, invalid_cnt = 0; cnt < lst->count; cnt ++ )
    {
        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid )
        {
            invalid_cnt++;
        }
        else
        {
            if ( 0 != mesh_test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) )
            {
                bc->valid = bfalse;
                invalid_cnt++;
            }
        }
    }

    // clean up the list
    if ( invalid_cnt > 0 )
    {
        breadcrumb_list_compact( lst );
    }

    // sort the values from lowest to highest
    if ( lst->count > 1 )
    {
        qsort( lst->lst, lst->count, sizeof( breadcrumb_t ), breadcrumb_cmp );
    }
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_list_last_valid( breadcrumb_list_t * lst )
{
    breadcrumb_t * retval = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    breadcrumb_list_validate( lst );

    if ( !breadcrumb_list_empty( lst ) )
    {
        retval = lst->lst + 0;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_list_newest( breadcrumb_list_t * lst )
{
    int cnt;

    Uint32         old_time = 0xFFFFFFFF;
    breadcrumb_t * old_ptr = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    for ( cnt = 0; cnt < lst->count; cnt ++ )
    {
        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        if ( NULL == old_ptr )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    for ( cnt++; cnt < lst->count; cnt++ )
    {
        int tmp;
        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        tmp = breadcrumb_cmp( old_ptr, bc );

        if ( tmp < 0 )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    return old_ptr;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_list_oldest( breadcrumb_list_t * lst )
{
    int cnt;

    Uint32         old_time = 0xFFFFFFFF;
    breadcrumb_t * old_ptr = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    for ( cnt = 0; cnt < lst->count; cnt ++ )
    {
        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        if ( NULL == old_ptr )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    for ( cnt++; cnt < lst->count; cnt++ )
    {
        int tmp;
        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        tmp = breadcrumb_cmp( old_ptr, bc );

        if ( tmp > 0 )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    return old_ptr;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_list_oldest_grid( breadcrumb_list_t * lst, Uint32 match_grid )
{
    int cnt;

    Uint32         old_time = 0xFFFFFFFF;
    breadcrumb_t * old_ptr = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    for ( cnt = 0; cnt < lst->count; cnt ++ )
    {
        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        if (( NULL == old_ptr ) && ( bc->grid == match_grid ) )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    for ( cnt++; cnt < lst->count; cnt++ )
    {
        int tmp;

        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        tmp = breadcrumb_cmp( old_ptr, bc );

        if (( tmp > 0 ) && ( bc->grid == match_grid ) )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    return old_ptr;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_list_alloc( breadcrumb_list_t * lst )
{
    breadcrumb_t * retval = NULL;

    if ( breadcrumb_list_full( lst ) )
    {
        breadcrumb_list_compact( lst );
    }

    if ( breadcrumb_list_full( lst ) )
    {
        retval = breadcrumb_list_oldest( lst );
    }
    else
    {
        retval = lst->lst + lst->count;
        lst->count++;
        retval->id = breadcrumb_guid++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t breadcrumb_list_add( breadcrumb_list_t * lst, breadcrumb_t * pnew )
{
    int cnt, invalid_cnt;

    bool_t retval;
    breadcrumb_t * pold, *ptmp;

    if ( NULL == lst || !lst->on ) return bfalse;

    if ( NULL == pnew ) return bfalse;

    for ( cnt = 0, invalid_cnt = 0; cnt < lst->count; cnt ++ )
    {
        breadcrumb_t * bc = lst->lst + cnt;

        if ( !bc->valid )
        {
            invalid_cnt++;
            break;
        }
    }

    if ( invalid_cnt > 0 )
    {
        breadcrumb_list_compact( lst );
    }

    // find the newest tile with the same grid position
    ptmp = breadcrumb_list_newest( lst );
    if ( NULL != ptmp && ptmp->valid )
    {
        if ( ptmp->grid == pnew->grid )
        {
            if ( INVALID_TILE == ptmp->grid )
            {
                // both are off the map, so determine the difference in distance
                if ( ABS( ptmp->pos.x - pnew->pos.x ) < GRID_SIZE && ABS( ptmp->pos.y - pnew->pos.y ) < GRID_SIZE )
                {
                    // not far enough apart
                    pold = ptmp;
                }
            }
            else
            {
                // the newest is on the same tile == the object hasn't moved
                pold = ptmp;
            }
        }
    }

    if ( breadcrumb_list_full( lst ) )
    {
        // the list is full, so we have to reuse an element

        // try the oldest element at this grid position
        pold = breadcrumb_list_oldest_grid( lst, pnew->grid );

        if ( NULL == pold )
        {
            // not found, so find the oldest breadcrumb
            pold = breadcrumb_list_oldest( lst );
        }
    }
    else
    {
        // the list is not full, so just allocate an element as normal

        pold = breadcrumb_list_alloc( lst );
    }

    // assign the data to the list element
    retval = bfalse;
    if ( NULL != pold )
    {
        *pold = *pnew;

        retval = btrue;
    }

    return retval;
}
