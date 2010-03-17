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

#include "char.inl"
#include "particle.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float hillslide       =  1.00f;
float slippyfriction  =  1.00f;
float airfriction     =  0.91f;
float waterfriction   =  0.80f;
float noslipfriction  =  0.91f;
float gravity         = -1.00f;
float platstick       =  0.01f;

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
    float diff;

    if ( NULL == tmin || NULL == tmax ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    diff = ovel2[index] - ovel1[index];
    //if ( diff == 0.0f ) return rv_fail;

    tolerance1 = (( OCT_Z == index ) && ( test_platform & PHYS_PLATFORM_OBJ1 ) ) ? PLATTOLERANCE : 0.0f;
    tolerance2 = (( OCT_Z == index ) && ( test_platform & PHYS_PLATFORM_OBJ2 ) ) ? PLATTOLERANCE : 0.0f;

    if ( 0.0f == tolerance1 && 0.0f == tolerance2 )
    {
        float time[4];

        time[0] = (( src1.mins[index] + opos1[index] ) - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[1] = (( src1.mins[index] + opos1[index] ) - ( src2.maxs[index] + opos2[index] ) ) / diff;
        time[2] = (( src1.maxs[index] + opos1[index] ) - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[3] = (( src1.maxs[index] + opos1[index] ) - ( src2.maxs[index] + opos2[index] ) ) / diff;

        *tmin = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        *tmax = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );
    }
    else if ( tolerance1 > 0.0f && tolerance2 > 0.0f )
    {
        float time[8];
        float tmp_min1, tmp_max1;
        float tmp_min2, tmp_max2;

        time[0] = (( src1.mins[index] + opos1[index] ) - ( src2.mins[index] - tolerance2 + opos2[index] ) ) / diff;
        time[1] = (( src1.mins[index] + opos1[index] ) - ( src2.maxs[index] + tolerance2 + opos2[index] ) ) / diff;
        time[2] = (( src1.maxs[index] + opos1[index] ) - ( src2.mins[index] - tolerance2 + opos2[index] ) ) / diff;
        time[3] = (( src1.maxs[index] + opos1[index] ) - ( src2.maxs[index] + tolerance2 + opos2[index] ) ) / diff;
        tmp_min1 = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        tmp_max1 = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );

        time[4] = (( src1.mins[index] - tolerance1 + opos1[index] ) - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[5] = (( src1.mins[index] - tolerance1 + opos1[index] ) - ( src2.maxs[index] + opos2[index] ) ) / diff;
        time[6] = (( src1.maxs[index] + tolerance1 + opos1[index] ) - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[7] = (( src1.maxs[index] + tolerance1 + opos1[index] ) - ( src2.maxs[index] + opos2[index] ) ) / diff;
        tmp_min2 = MIN( MIN( time[4], time[5] ), MIN( time[6], time[7] ) );
        tmp_max2 = MAX( MAX( time[4], time[5] ), MAX( time[6], time[7] ) );

        *tmin = MIN( tmp_min1, tmp_min2 );
        *tmax = MAX( tmp_max1, tmp_max2 );
    }
    else
    {
        float time[4];

        time[0] = (( src1.mins[index] - tolerance1 + opos1[index] ) - ( src2.mins[index] - tolerance2 + opos2[index] ) ) / diff;
        time[1] = (( src1.mins[index] - tolerance1 + opos1[index] ) - ( src2.maxs[index] + tolerance2 + opos2[index] ) ) / diff;
        time[2] = (( src1.maxs[index] + tolerance1 + opos1[index] ) - ( src2.mins[index] - tolerance2 + opos2[index] ) ) / diff;
        time[3] = (( src1.maxs[index] + tolerance1 + opos1[index] ) - ( src2.maxs[index] + tolerance2 + opos2[index] ) ) / diff;

        *tmin = MIN( MIN( time[0], time[1] ), MIN( time[2], time[3] ) );
        *tmax = MAX( MAX( time[0], time[1] ), MAX( time[2], time[3] ) );
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
bool_t phys_intersect_oct_bb( oct_bb_t src1_orig, fvec3_t pos1, fvec3_t vel1, oct_bb_t src2_orig, fvec3_t pos2, fvec3_t vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax )
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

    // handle optional parameters
    if ( NULL == tmin ) tmin = &local_tmin;
    if ( NULL == tmax ) tmax = &local_tmax;

    //// do the objects interact at the very beginning of the update?
    //if ( test_interaction_2( src1_orig, pos2, src2_orig, pos2, test_platform ) )
    //{
    //    oct_bb_t tmp1, tmp2;

    //    oct_bb_add_vector( src1_orig, pos1, &tmp1 );
    //    oct_bb_add_vector( src2_orig, pos2, &tmp2 );

    //    oct_bb_intersection( tmp1, tmp2, pdst );

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
    for ( index = 0; index < OCT_COUNT; index ++ )
    {
        egoboo_rv retval;

        retval = oct_bb_intersect_index( index, src1_orig, opos1, ovel1, src2_orig, opos2, ovel2, test_platform, &tmp_min, &tmp_max );
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

    tmp_min = CLIP( *tmin, 0.0f, 1.0f );
    tmp_max = CLIP( *tmax, 0.0f, 1.0f );

    // shift the cource bounding boxes to be centered on the given positions
    oct_bb_add_vector( src1_orig, pos1, &src1 );
    oct_bb_add_vector( src2_orig, pos2, &src2 );

    // determine the expanded collision volumes for both objects
    phys_expand_oct_bb( src1, vel1, tmp_min, tmp_max, &exp1 );
    phys_expand_oct_bb( src2, vel2, tmp_min, tmp_max, &exp2 );

    // determine the intersection of these two volumes
    oct_bb_intersection( exp1, exp2, pdst );

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
            oct_bb_intersection( src1, src2, pdst );
        }

        return btrue;
    }

    // convert the position and velocity vectors to octagonal format
    oct_vec_ctor( ovel1, vel1 );
    oct_vec_ctor( opos1, pos1 );

    oct_vec_ctor( ovel2, vel2 );
    oct_vec_ctor( opos2, pos2 );

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
    phys_expand_oct_bb( src1, vel1, *tmin, *tmax, &exp1 );
    phys_expand_oct_bb( src2, vel2, *tmin, *tmax, &exp2 );

    // determine the intersection of these two volumes
    oct_bb_intersection( exp1, exp2, &intersection );

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
bool_t phys_expand_oct_bb( oct_bb_t src, fvec3_t vel, float tmin, float tmax, oct_bb_t * pdst )
{
    /// @details BB@> use the velocity of an object and its oct_bb_t to determine the
    ///               amount of territory that an object will cover in the range [tmin,tmax].
    ///               One update equals [tmin,tmax] == [0,1].

    float abs_vel;
    oct_bb_t tmp_min, tmp_max;

    abs_vel = ABS( vel.x ) + ABS( vel.y ) + ABS( vel.z );
    if ( 0.0f == abs_vel )
    {
        if ( NULL != pdst )
        {
            *pdst = src;
        }
        return btrue;
    }

    // determine the bounding volume at t == tmin
    if ( tmin == 0.0f )
    {
        tmp_min = src;
    }
    else
    {
        fvec3_t pos_min;

        pos_min.x = vel.x * tmin;
        pos_min.y = vel.y * tmin;
        pos_min.z = vel.z * tmin;

        // adjust the bounding box to take in the position at the next step
        if ( !oct_bb_add_vector( src, pos_min, &tmp_min ) ) return bfalse;
    }

    // determine the bounding volume at t == tmax
    if ( tmax == 0.0f )
    {
        tmp_max = src;
    }
    else
    {
        fvec3_t pos_max;

        pos_max.x = vel.x * tmax;
        pos_max.y = vel.y * tmax;
        pos_max.z = vel.z * tmax;

        // adjust the bounding box to take in the position at the next step
        if ( !oct_bb_add_vector( src, pos_max, &tmp_max ) ) return bfalse;
    }

    // determine bounding box for the range of times
    if ( !oct_bb_union( tmp_min, tmp_max, pdst ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t phys_expand_chr_bb( chr_t * pchr, float tmin, float tmax, oct_bb_t * pdst )
{
    /// @details BB@> use the object velocity to figure out where the volume that the character will
    ///               occupy during this update. Use the loser chr_prt_cv and include extra height if
    ///               it is a platform.

    oct_bb_t tmp_oct1, tmp_oct2;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // copy the volume
    tmp_oct1 = pchr->chr_prt_cv;

    if ( pchr->platform )
    {
        // expand the interaction range so that we will correctly detect interactions with platforms
        tmp_oct1.maxs[OCT_Z] += PLATTOLERANCE;
    }

    // add in the current position to the bounding volume
    oct_bb_add_vector( tmp_oct1, pchr->pos, &tmp_oct2 );

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb( tmp_oct2, pchr->vel, tmin, tmax, pdst );
}

//--------------------------------------------------------------------------------------------
bool_t phys_expand_prt_bb( prt_t * pprt, float tmin, float tmax, oct_bb_t * pdst )
{
    /// @details BB@> use the object velocity to figure out where the volume that the particle will
    ///               occupy during this update

    oct_bb_t tmp_oct;

    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // add in the current position to the bounding volume
    oct_bb_add_vector( pprt->chr_prt_cv, pprt->pos, &tmp_oct );

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb( tmp_oct, pprt->vel, tmin, tmax, pdst );
}