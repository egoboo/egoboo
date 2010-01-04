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

#include "physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float hillslide       =  1.00f;
float slippyfriction  =  1.00f;
float airfriction     =  0.91f;
float waterfriction   =  0.80f;
float noslipfriction  =  0.91f;
float gravity         = -1.00f;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t get_depth_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_close_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_close_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_close_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_close_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    bool_t valid;

    if ( NULL == depth ) return bfalse;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    valid = btrue;
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        float ftmp1 = MIN(( ob[cnt] + cv_b.maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b.mins[cnt] ) );
        float ftmp2 = MIN(( oa[cnt] + cv_a.maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a.mins[cnt] ) );
        depth[cnt] = MAX( ftmp1, ftmp2 );

        if ( depth[cnt] <= 0.0f )
        {
            valid = bfalse;
            if ( break_out ) return bfalse;
        }
    }

    // treat the z coordinate the same as always
    depth[OCT_Z]  = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
                    MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    if ( depth[OCT_Z] <= 0.0f )
    {
        valid = bfalse;
        if ( break_out ) return bfalse;
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
bool_t get_depth_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t break_out, oct_vec_t depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    bool_t valid;

    if ( NULL == depth ) return bfalse;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    valid = btrue;
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        depth[cnt]  = MIN( cv_b.maxs[cnt] + ob[cnt], cv_a.maxs[cnt] + oa[cnt] ) -
                      MAX( cv_b.mins[cnt] + ob[cnt], cv_a.mins[cnt] + oa[cnt] );

        if ( depth[cnt] <= 0.0f )
        {
            valid = bfalse;
            if ( break_out ) return bfalse;
        }
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t test_interaction_close_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_close_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_0( bumper_t bump_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_close_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_close_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_1( oct_bb_t cv_a, fvec3_t pos_a, bumper_t bump_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_close_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    float depth;
    oct_vec_t oa, ob;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        float ftmp1 = MIN(( ob[cnt] + cv_b.maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b.mins[cnt] ) );
        float ftmp2 = MIN(( oa[cnt] + cv_a.maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a.mins[cnt] ) );
        depth = MAX( ftmp1, ftmp2 );
        if ( depth <= 0.0f ) return bfalse;
    }

    // treat the z coordinate the same as always
    depth = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
            MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    return test_platform ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f );
}

//--------------------------------------------------------------------------------------------
bool_t test_interaction_2( oct_bb_t cv_a, fvec3_t pos_a, oct_bb_t cv_b, fvec3_t pos_b, bool_t test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    float depth;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        depth  = MIN( cv_b.maxs[cnt] + ob[cnt], cv_a.maxs[cnt] + oa[cnt] ) -
                 MAX( cv_b.mins[cnt] + ob[cnt], cv_a.mins[cnt] + oa[cnt] );

        if ( depth <= 0.0f ) return bfalse;
    }

    // treat the z coordinate the same as always
    depth = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
            MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    return test_platform ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f );
}

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
    if ( diff == 0.0f ) return rv_fail;

    tolerance1 = (( OCT_Z == index ) && ( test_platform & 1 ) ) ? PLATTOLERANCE : 0.0f;
    tolerance2 = (( OCT_Z == index ) && ( test_platform & 2 ) ) ? PLATTOLERANCE : 0.0f;

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

    tolerance1 = (( OCT_Z == index ) && ( test_platform & 1 ) ) ? PLATTOLERANCE : 0.0f;
    tolerance2 = (( OCT_Z == index ) && ( test_platform & 2 ) ) ? PLATTOLERANCE : 0.0f;

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
bool_t oct_bb_intersect( oct_bb_t src1, fvec3_t pos1, fvec3_t vel1, oct_bb_t src2, fvec3_t pos2, fvec3_t vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax )
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

        retval = oct_bb_intersect_index( index, src1, opos1, ovel1, src2, opos2, ovel2, test_platform, &tmp_min, &tmp_max );
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
    oct_bb_expand( src1, vel1, *tmin, *tmax, &exp1 );
    oct_bb_expand( src2, vel2, *tmin, *tmax, &exp2 );

    // determine the intersection of these two volumes
    oct_bb_intersection( exp1, exp2, &intersection );

    // check to see if there is any possibility of interaction at all
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        if ( intersection.mins[cnt] > intersection.maxs[cnt] ) return bfalse;
    }

    tolerance = test_platform ? PLATTOLERANCE : 0.0f;
    if ( intersection.mins[OCT_Z] > intersection.maxs[OCT_Z] + tolerance ) return bfalse;

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
    oct_bb_expand( src1, vel1, *tmin, *tmax, &exp1 );
    oct_bb_expand( src2, vel2, *tmin, *tmax, &exp2 );

    // determine the intersection of these two volumes
    oct_bb_intersection( exp1, exp2, &intersection );

    // check to see if there is any possibility of interaction at all
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        if ( intersection.mins[cnt] > intersection.maxs[cnt] ) return bfalse;
    }

    tolerance = test_platform ? PLATTOLERANCE : 0.0f;
    if ( intersection.mins[OCT_Z] > intersection.maxs[OCT_Z] + tolerance ) return bfalse;

    return btrue;
}
