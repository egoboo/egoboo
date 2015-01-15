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

/// @file game/physics.c

#include "game/physics.h"

#include "game/game.h"

#include "game/char.h"
#include "game/particle.h"
#include "game/mesh.h"

#include "game/ChrList.h"
#include "game/PrtList.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float   hillslide       =  1.00f;
float   slippyfriction  =  1.00f;
float   airfriction     =  0.91f;
float   waterfriction   =  0.80f;
float   noslipfriction  =  0.91f;
float   gravity         = -1.00f;
float   platstick       =  0.01f;
fvec3_t windspeed       = fvec3_t::zero;
fvec3_t waterspeed      = fvec3_t::zero;

static int breadcrumb_guid = 0;

const float air_friction = 0.9868f;
const float ice_friction = 0.9738f;  // the square of air_friction

static egolib_rv phys_intersect_oct_bb_index( int index, const oct_bb_t * src1, const oct_vec_t ovel1, const oct_bb_t *  src2, const oct_vec_t ovel2, int test_platform, float *tmin, float *tmax );
static egolib_rv phys_intersect_oct_bb_close_index( int index, const oct_bb_t * src1, const oct_vec_t ovel1, const oct_bb_t *  src2, const oct_vec_t ovel2, int test_platform, float *tmin, float *tmax );

/// @brief A test to determine whether two "fast moving" objects are interacting within a frame.
///        Designed to determine whether a bullet particle will interact with character.
static bool phys_intersect_oct_bb_close( const oct_bb_t * src1_orig, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t *  src2_orig, const fvec3_base_t pos2, const fvec3_base_t vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax );
static bool phys_estimate_depth( const oct_vec_t * podepth, const float exponent, fvec3_t& nrm, float * depth );
static float phys_get_depth( const oct_vec_t * podepth, const fvec3_t& nrm );
static bool phys_warp_normal( const float exponent, fvec3_t& nrm );
static bool phys_get_pressure_depth( const oct_bb_t * pbb_a, const oct_bb_t * pbb_b, oct_vec_t * podepth );
static bool phys_get_collision_depth( const oct_bb_t * pbb_a, const oct_bb_t * pbb_b, oct_vec_t * podepth );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool phys_get_collision_depth( const oct_bb_t * pbb_a, const oct_bb_t * pbb_b, oct_vec_t * podepth )
{
    int cnt;
    float fdiff, fdepth;
    bool retval;

    oct_bb_t otmp;
    oct_vec_t opos_a, opos_b;

    if ( NULL == podepth || NULL == *podepth ) return false;

    oct_vec_self_clear( podepth );

    // are the initial volumes any good?
    if ( NULL == pbb_a || pbb_a->empty ) return false;
    if ( NULL == pbb_b || pbb_b->empty ) return false;

    // is there any overlap?
    if ( rv_success != oct_bb_intersection( pbb_a, pbb_b, &otmp ) )
    {
        return false;
    }

    // estimate the "cm position" of the objects by the bounding volumes
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        opos_a[cnt] = ( pbb_a->maxs[cnt] + pbb_a->mins[cnt] ) * 0.5f;
        opos_b[cnt] = ( pbb_b->maxs[cnt] + pbb_b->mins[cnt] ) * 0.5f;
    }

    // find the (signed) depth in each dimension
    retval = true;
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        fdiff  = opos_b[cnt] - opos_a[cnt];
        fdepth = otmp.maxs[cnt] - otmp.mins[cnt];

        // if the measured depth is less than zero, or the difference in positions
        // is ambiguous, this algorithm fails
        if ( fdepth <= 0.0f || 0.0f == fdiff ) retval = false;

        ( *podepth )[cnt] = ( fdiff < 0.0f ) ? -fdepth : fdepth;
    }
    ( *podepth )[OCT_XY] *= INV_SQRT_TWO;
    ( *podepth )[OCT_YX] *= INV_SQRT_TWO;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool phys_get_pressure_depth( const oct_bb_t * pbb_a, const oct_bb_t * pbb_b, oct_vec_t * podepth )
{
    int cnt;
    bool rv;

    if ( NULL == podepth || NULL == *podepth ) return false;

    oct_vec_self_clear( podepth );

    if ( NULL == pbb_a || NULL == pbb_b ) return false;

    // assume the best
    rv = true;

    // scan through the dimensions of the oct_bbs
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        float diff1 = 0.0f;
        float diff2 = 0.0f;

        diff1 = pbb_a->maxs[cnt] - pbb_b->mins[cnt];
        diff2 = pbb_b->maxs[cnt] - pbb_a->mins[cnt];

        if ( diff1 < 0.0f || diff2 < 0.0f )
        {
            // this case will only happen if there is no overlap in one of the dimensions,
            // meaning there was a bad collision detection... it should NEVER happen.
            // In any case this math still generates the proper direction for
            // the normal pointing away from b.
            if ( ABS( diff1 ) < ABS( diff2 ) )
            {
                ( *podepth )[cnt] = diff1;
            }
            else
            {
                ( *podepth )[cnt] = -diff2;
            }

            rv = false;
        }
        else if ( diff1 < diff2 )
        {
            ( *podepth )[cnt] = -diff1;
        }
        else
        {
            ( *podepth )[cnt] = diff2;
        }
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool phys_warp_normal( const float exponent, fvec3_t& nrm )
{
    // use the exponent to warp the normal into a cylinder-like shape, if needed

    float length_hrz_2, length_vrt_2;

    if ( 1.0f == exponent ) return true;

    if ( 0.0f == fvec3_length_abs( nrm ) ) return false;

    length_hrz_2 = fvec2_length_2( fvec2_t(nrm[kX],nrm[kY]) );
    length_vrt_2 = fvec3_length_2( nrm ) - length_hrz_2;

    nrm[kX] = nrm[kX] * POW( length_hrz_2, 0.5f * ( exponent - 1.0f ) );
    nrm[kY] = nrm[kY] * POW( length_hrz_2, 0.5f * ( exponent - 1.0f ) );
    nrm[kZ] = nrm[kZ] * POW( length_vrt_2, 0.5f * ( exponent - 1.0f ) );

    // normalize the normal
	fvec3_self_normalize(nrm);
    return fvec3_length(nrm) >= 0.0f;
}

//--------------------------------------------------------------------------------------------
float phys_get_depth( const oct_vec_t * podepth, const fvec3_t& nrm )
{
    const float max_val = 1e6;

    int cnt;
    float depth, ftmp;
    oct_vec_t onrm;

    if ( NULL == podepth || NULL == *podepth ) return 0.0f;

    if ( 0.0f == fvec3_length_abs( nrm ) ) return max_val;

    // convert the normal into an oct_vec_t
    oct_vec_ctor( onrm, nrm );
    onrm[OCT_XY] *= INV_SQRT_TWO;
    onrm[OCT_YX] *= INV_SQRT_TWO;

    // calculate the minimum depth in each dimension
    depth = max_val;
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        if ( 0.0f == ( *podepth )[cnt] )
        {
            depth = 0.0f;
            break;
        }

        if ( 0.0f != onrm[cnt] )
        {
            ftmp = ( *podepth )[cnt] / onrm[cnt];
            if ( ftmp <= 0.0f )
            {
                depth = 0.0f;
                break;
            }

            depth = std::min( depth, ftmp );
        }
    }

    return depth;
}

//--------------------------------------------------------------------------------------------
bool phys_estimate_depth( const oct_vec_t * podepth, const float exponent, fvec3_t& nrm, float * depth )
{
    // use the given (signed) podepth info to make a normal vector, and measure
    // the shortest distance to the border

    float   tmin_aa, tmin_diag, ftmp, tmin;
    fvec3_t nrm_aa, nrm_diag;

    bool rv;

    if ( NULL == podepth )

        // first do the aa axes
        fvec3_self_clear( nrm_aa.v );

    if ( 0.0f != ( *podepth )[OCT_X] ) nrm_aa.x = 1.0f / ( *podepth )[OCT_X];
    if ( 0.0f != ( *podepth )[OCT_Y] ) nrm_aa.y = 1.0f / ( *podepth )[OCT_Y];
    if ( 0.0f != ( *podepth )[OCT_Z] ) nrm_aa.z = 1.0f / ( *podepth )[OCT_Z];

    if ( 1.0f == exponent )
    {
        fvec3_self_normalize( nrm_aa );
    }
    else
    {
        phys_warp_normal( exponent, nrm_aa );
    }

    // find a minimum distance
    tmin_aa = 1e6;

    if ( nrm_aa.x != 0.0f )
    {
        ftmp = ( *podepth )[OCT_X] / nrm_aa.x;
        ftmp = std::max( 0.0f, ftmp );
        tmin_aa = std::min( tmin_aa, ftmp );
    }

    if ( nrm_aa.y != 0.0f )
    {
        ftmp = ( *podepth )[OCT_Y] / nrm_aa.y;
        ftmp = std::max( 0.0f, ftmp );
        tmin_aa = std::min( tmin_aa, ftmp );
    }

    if ( nrm_aa.z != 0.0f )
    {
        ftmp = ( *podepth )[OCT_Z] / nrm_aa.z;
        ftmp = std::max( 0.0f, ftmp );
        tmin_aa = std::min( tmin_aa, ftmp );
    }

    if ( tmin_aa <= 0.0f || tmin_aa >= 1e6 ) return false;

    // next do the diagonal axes
    fvec3_self_clear( nrm_diag.v );

    if ( 0.0f != ( *podepth )[OCT_XY] ) nrm_diag.x = 1.0f / (( *podepth )[OCT_XY] * INV_SQRT_TWO );
    if ( 0.0f != ( *podepth )[OCT_YX] ) nrm_diag.y = 1.0f / (( *podepth )[OCT_YX] * INV_SQRT_TWO );
    if ( 0.0f != ( *podepth )[OCT_Z ] ) nrm_diag.z = 1.0f / ( *podepth )[OCT_Z];

    if ( 1.0f == exponent )
    {
        fvec3_self_normalize( nrm_diag );
    }
    else
    {
        phys_warp_normal( exponent, nrm_diag );
    }

    // find a minimum distance
    tmin_diag = 1e6;

    if ( nrm_diag.x != 0.0f )
    {
        ftmp = INV_SQRT_TWO * ( *podepth )[OCT_XY] / nrm_diag.x;
        ftmp = std::max( 0.0f, ftmp );
        tmin_diag = std::min( tmin_diag, ftmp );
    }

    if ( nrm_diag.y != 0.0f )
    {
        ftmp = INV_SQRT_TWO * ( *podepth )[OCT_YX] / nrm_diag.y;
        ftmp = std::max( 0.0f, ftmp );
        tmin_diag = std::min( tmin_diag, ftmp );
    }

    if ( nrm_diag.z != 0.0f )
    {
        ftmp = ( *podepth )[OCT_Z] / nrm_diag.z;
        ftmp = std::max( 0.0f, ftmp );
        tmin_diag = std::min( tmin_diag, ftmp );
    }

    if ( tmin_diag <= 0.0f || tmin_diag >= 1e6 ) return false;

    if ( tmin_aa < tmin_diag )
    {
        tmin = tmin_aa;
		nrm = nrm_aa;
#if 0
        fvec3_base_copy( nrm, nrm_aa.v );
#endif
    }
    else
    {
        tmin = tmin_diag;

        // !!!! rotate the diagonal axes onto the axis aligned ones !!!!!
        nrm[kX] = ( nrm_diag.x - nrm_diag.y ) * INV_SQRT_TWO;
        nrm[kY] = ( nrm_diag.x + nrm_diag.y ) * INV_SQRT_TWO;
        nrm[kZ] = nrm_diag.z;
    }

    // normalize this normal
	fvec3_self_normalize(nrm);
    rv = fvec3_length(nrm) > 0.0f;

    // find the depth in the direction of the normal, if possible
    if ( rv && NULL != depth )
    {
        *depth = tmin;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool phys_estimate_collision_normal( const oct_bb_t * pobb_a, const oct_bb_t * pobb_b, const float exponent, oct_vec_t * podepth, fvec3_t& nrm, float * depth )
{
    // estimate the normal for collision volumes that are partially overlapping

    bool use_pressure;

    // is everything valid?
    if ( NULL == pobb_a || NULL == pobb_b ) return false;

    // do we need to use the more expensive algorithm?
    use_pressure = false;
    if ( oct_bb_lhs_contains_rhs( pobb_a, pobb_b ) )
    {
        use_pressure = true;
    }
    else if ( oct_bb_lhs_contains_rhs( pobb_b, pobb_a ) )
    {
        use_pressure = true;
    }

    if ( !use_pressure )
    {
        // try to get the collision depth using the given oct_bb's
        if ( !phys_get_collision_depth( pobb_a, pobb_b, podepth ) )
        {
            use_pressure = true;
        }
    }

    if ( use_pressure )
    {
        return phys_estimate_pressure_normal( pobb_a, pobb_b, exponent, podepth, nrm, depth );
    }

    return phys_estimate_depth( podepth, exponent, nrm, depth );
}

//--------------------------------------------------------------------------------------------
bool phys_estimate_pressure_normal( const oct_bb_t * pobb_a, const oct_bb_t * pobb_b, const float exponent, oct_vec_t * podepth, fvec3_t& nrm, float * depth )
{
    // use a more robust algorithm to get the normal no matter how the 2 volumes are
    // related

    float     loc_tmin;
    fvec3_t   loc_nrm;
    oct_vec_t loc_odepth;

    bool rv;

    // handle "optional" parameters
    if ( NULL == depth ) depth = &loc_tmin;
#if 0
    if ( NULL == nrm ) nrm = loc_nrm.v;
#endif
    if ( NULL == podepth || NULL == *podepth ) podepth = &loc_odepth;

    if ( NULL == pobb_a || NULL == pobb_b ) return false;

    // calculate the direction of the nearest way out for each octagonal axis
    rv = phys_get_pressure_depth( pobb_a, pobb_b, podepth );
    if ( !rv ) return false;

    return phys_estimate_depth( podepth, exponent, nrm, depth );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv phys_intersect_oct_bb_index( int index, const oct_bb_t * src1, const oct_vec_t ovel1, const oct_bb_t * src2, const oct_vec_t ovel2, int test_platform, float *tmin, float *tmax )
{
    float vdiff;
    float src1_min, src1_max;
    float src2_min, src2_max;

    if ( NULL == tmin || NULL == tmax ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    vdiff = ovel2[index] - ovel1[index];
    if ( 0.0f == vdiff ) return rv_fail;

    src1_min = src1->mins[index];
    src1_max = src1->maxs[index];
    src2_min = src2->mins[index];
    src2_max = src2->maxs[index];

    if ( OCT_Z != index )
    {
        bool close_test_1, close_test_2;

        // is there any possibility of the 2 objects acting as a platform pair
        close_test_1 = HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ1 );
        close_test_2 = HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ2 );

        // only do a close test if the object's feet are above the platform
        close_test_1 = close_test_1 && ( src1->mins[OCT_Z] > src2->maxs[OCT_Z] );
        close_test_2 = close_test_2 && ( src2->mins[OCT_Z] > src1->maxs[OCT_Z] );

        if ( !close_test_1 && !close_test_2 )
        {
            // NEITHER ia a platform
            float time[4];

            time[0] = ( src1_min - src2_min ) / vdiff;
            time[1] = ( src1_min - src2_max ) / vdiff;
            time[2] = ( src1_max - src2_min ) / vdiff;
            time[3] = ( src1_max - src2_max ) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
        else
        {
            return phys_intersect_oct_bb_close_index( index, src1, ovel1, src2, ovel2, test_platform, tmin, tmax );
        }
    }
    else /* OCT_Z == index */
    {
        float tolerance_1, tolerance_2;
        float plat_min, plat_max;

        // add in a tolerance into the vertical direction for platforms
        tolerance_1 = HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ1 ) ? PLATTOLERANCE : 0.0f;
        tolerance_2 = HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ2 ) ? PLATTOLERANCE : 0.0f;

        if ( 0.0f == tolerance_1 && 0.0f == tolerance_2 )
        {
            // NEITHER ia a platform

            float time[4];

            time[0] = ( src1_min - src2_min ) / vdiff;
            time[1] = ( src1_min - src2_max ) / vdiff;
            time[2] = ( src1_max - src2_min ) / vdiff;
            time[3] = ( src1_max - src2_max ) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
        else if ( 0.0f == tolerance_1 )
        {
            float time[4];

            // obj2 is platform
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = ( src1_min - plat_min ) / vdiff;
            time[1] = ( src1_min - plat_max ) / vdiff;
            time[2] = ( src1_max - plat_min ) / vdiff;
            time[3] = ( src1_max - plat_max ) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
        else if ( 0.0f == tolerance_2 )
        {
            // ONLY src1 is a platform
            float time[4];

            // obj1 is platform
            plat_min = src1_min;
            plat_max = src1_max + tolerance_2;

            time[0] = ( plat_min - src2_min ) / vdiff;
            time[1] = ( plat_min - src2_max ) / vdiff;
            time[2] = ( plat_max - src2_min ) / vdiff;
            time[3] = ( plat_max - src2_max ) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }

        else if ( tolerance_1 > 0.0f && tolerance_2 > 0.0f )
        {
            // BOTH are platforms
            // they cannot both act as plaforms at the same time, so do 8 tests

            float time[8];
            float tmp_min1, tmp_max1;
            float tmp_min2, tmp_max2;

            // obj2 is platform
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = ( src1_min - plat_min ) / vdiff;
            time[1] = ( src1_min - plat_max ) / vdiff;
            time[2] = ( src1_max - plat_min ) / vdiff;
            time[3] = ( src1_max - plat_max ) / vdiff;
            tmp_min1 = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            tmp_max1 = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );

            // obj1 is platform
            plat_min = src1_min;
            plat_max = src1_max + tolerance_2;

            time[4] = ( plat_min - src2_min ) / vdiff;
            time[5] = ( plat_min - src2_max ) / vdiff;
            time[6] = ( plat_max - src2_min ) / vdiff;
            time[7] = ( plat_max - src2_max ) / vdiff;
            tmp_min2 = std::min( std::min( time[4], time[5] ), std::min( time[6], time[7] ) );
            tmp_max2 = std::max( std::max( time[4], time[5] ), std::max( time[6], time[7] ) );

            *tmin = std::min( tmp_min1, tmp_min2 );
            *tmax = std::max( tmp_max1, tmp_max2 );
        }
    }

    // normalize the results for the diagonal directions
    if ( OCT_XY == index || OCT_YX == index )
    {
        *tmin *= INV_SQRT_TWO;
        *tmax *= INV_SQRT_TWO;
    }

    if ( *tmax <= *tmin ) return rv_fail;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool phys_intersect_oct_bb( const oct_bb_t * src1_orig, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t * src2_orig, const fvec3_t& pos2, const fvec3_t& vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax )
{
    /// @author BB
	/// @details A test to determine whether two "fast moving" objects are interacting within a frame.
	///               Designed to determine whether a bullet particle will interact with character.


    oct_bb_t src1, src2;
    oct_bb_t exp1, exp2;

    oct_vec_t opos1, opos2;
    oct_vec_t ovel1, ovel2;

    int    index;
    bool found;
    float  local_tmin, local_tmax;

    int    failure_count = 0;
    bool failure[OCT_COUNT];

    // handle optional parameters
    if ( NULL == tmin ) tmin = &local_tmin;
    if ( NULL == tmax ) tmax = &local_tmax;

    // convert the position and velocity vectors to octagonal format
    oct_vec_ctor( ovel1, vel1 );
    oct_vec_ctor( opos1, pos1 );

    oct_vec_ctor( ovel2, vel2 );
    oct_vec_ctor( opos2, pos2 );

    // shift the bounding boxes to their starting positions
    oct_bb_add_ovec( src1_orig, opos1, &src1 );
    oct_bb_add_ovec( src2_orig, opos2, &src2 );

    found = false;
    *tmin = +1.0e6;
    *tmax = -1.0e6;
    if ( fvec3_dist_abs( vel1, vel2 ) < 1.0e-6 )
    {
        // no relative motion, so avoid the loop to save time
        failure_count = OCT_COUNT;
    }
    else
    {
        // cycle through the coordinates to see when the two volumes might coincide
        for ( index = 0; index < OCT_COUNT; index++ )
        {
            egolib_rv retval;

            if ( ABS( ovel1[index] - ovel2[index] ) < 1.0e-6 )
            {
                failure[index] = true;
                failure_count++;
            }
            else
            {
                float tmp_min = 0.0f, tmp_max = 0.0f;

                retval = phys_intersect_oct_bb_index( index, &src1, ovel1, &src2, ovel2, test_platform, &tmp_min, &tmp_max );

                // check for overflow
                if ( ieee32_bad( tmp_min ) || ieee32_bad( tmp_max ) )
                {
                    retval = rv_fail;
                }

                if ( rv_fail == retval )
                {
                    // This case will only occur if the objects are not moving relative to each other.

                    failure[index] = true;
                    failure_count++;
                }
                else if ( rv_success == retval )
                {
                    failure[index] = false;

                    if ( !found )
                    {
                        *tmin = tmp_min;
                        *tmax = tmp_max;
                        found = true;
                    }
                    else
                    {
                        *tmin = std::max( *tmin, tmp_min );
                        *tmax = std::min( *tmax, tmp_max );
                    }

                    // check the values vs. reasonable bounds
                    if ( *tmax <= *tmin ) return false;
                    if ( *tmin > 1.0f || *tmax < 0.0f ) return false;
                }
            }
        }
    }

    if ( OCT_COUNT == failure_count )
    {
        // No relative motion on any axis.
        // Just say that they are interacting for the whole frame

        *tmin = 0.0f;
        *tmax = 1.0f;

        // determine the intersection of these two expanded volumes (for this frame)
        oct_bb_intersection( &src1, &src2, pdst );
    }
    else
    {
        float tmp_min, tmp_max;

        // check to see if there the intersection times make any sense
        if ( *tmax <= *tmin ) return false;

        // check whether there is any overlap this frame
        if ( *tmin >= 1.0f || *tmax <= 0.0f ) return false;

        // clip the interaction time to just one frame
        tmp_min = CLIP( *tmin, 0.0f, 1.0f );
        tmp_max = CLIP( *tmax, 0.0f, 1.0f );

        // determine the expanded collision volumes for both objects (for this frame)
        phys_expand_oct_bb( &src1, vel1, tmp_min, tmp_max, &exp1 );
        phys_expand_oct_bb( &src2, vel2, tmp_min, tmp_max, &exp2 );

        // determine the intersection of these two expanded volumes (for this frame)
        oct_bb_intersection( &exp1, &exp2, pdst );
    }

    if ( 0 != test_platform )
    {
        pdst->maxs[OCT_Z] += PLATTOLERANCE;
        oct_bb_validate( pdst );
    }

    if ( pdst->empty ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv phys_intersect_oct_bb_close_index( int index, const oct_bb_t * src1, const oct_vec_t ovel1, const oct_bb_t * src2, const oct_vec_t ovel2, int test_platform, float *tmin, float *tmax )
{
    float vdiff;
    float opos1, opos2;
    float src1_min, src1_max;
    float src2_min, src2_max;

    if ( NULL == tmin || NULL == tmax ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    vdiff = ovel2[index] - ovel1[index];
    if ( 0.0f == vdiff ) return rv_fail;

    src1_min = src1->mins[index];
    src1_max = src1->maxs[index];
    opos1 = ( src1_min + src1_max ) * 0.5f;

    src2_min = src2->mins[index];
    src2_max = src2->maxs[index];
    opos2 = ( src2_min + src2_max ) * 0.5f;

    if ( OCT_Z != index )
    {
        bool platform_1;
        bool platform_2;

        platform_1 = HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ1 );
        platform_2 = HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ2 );

        if ( !platform_1 && !platform_2 )
        {
            // NEITHER ia a platform
            // use the eqn. from phys_intersect_oct_bb_index()

            float time[4];

            time[0] = ( src1_min - src2_min ) / vdiff;
            time[1] = ( src1_min - src2_max ) / vdiff;
            time[2] = ( src1_max - src2_min ) / vdiff;
            time[3] = ( src1_max - src2_max ) / vdiff;

			*tmin = std::min({ time[0], time[1], time[2], time[3] });
			*tmax = std::max({ time[0], time[1], time[2], time[3] });
        }
        else if ( platform_1 && !platform_2 )
        {
            float time[2];

            // obj1 is the platform
            time[0] = ( src1_min - opos2 ) / vdiff;
            time[1] = ( src1_max - opos2 ) / vdiff;

            *tmin = std::min( time[0], time[1] );
            *tmax = std::max( time[0], time[1] );
        }
        else if ( !platform_1 && platform_2 )
        {
            float time[2];

            // obj2 is the platform
            time[0] = ( opos1 - src2_min ) / vdiff;
            time[1] = ( opos1 - src2_max ) / vdiff;

            *tmin = std::min( time[0], time[1] );
            *tmax = std::max( time[0], time[1] );
        }
        else
        {
            // BOTH are platforms. must check all possibilities
            float time[4];

            // object 1 is the platform
            time[0] = ( src1_min - opos2 ) / vdiff;
            time[1] = ( src1_max - opos2 ) / vdiff;

            // object 2 is the platform
            time[2] = ( opos1 - src2_min ) / vdiff;
            time[3] = ( opos1 - src2_max ) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
    }
    else /* OCT_Z == index */
    {
        float tolerance_1, tolerance_2;

        float plat_min, plat_max;
        float obj_pos;

        tolerance_1 =  HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ1 ) ? PLATTOLERANCE : 0.0f;
        tolerance_2 =  HAS_SOME_BITS( test_platform, PHYS_PLATFORM_OBJ2 ) ? PLATTOLERANCE : 0.0f;

        if ( 0.0f == tolerance_1 && 0.0f == tolerance_2 )
        {
            // NEITHER ia a platform
            // use the eqn. from phys_intersect_oct_bb_index()

            float time[4];

            time[0] = ( src1_min - src2_min ) / vdiff;
            time[1] = ( src1_min - src2_max ) / vdiff;
            time[2] = ( src1_max - src2_min ) / vdiff;
            time[3] = ( src1_max - src2_max ) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
        else if ( 0.0f != tolerance_1 && 0.0f == tolerance_2 )
        {
            float time[2];

            // obj1 is the platform
            obj_pos  = src2_min;
            plat_min = src1_min;
            plat_max = src1_max + tolerance_1;

            time[0] = ( plat_min - obj_pos ) / vdiff;
            time[1] = ( plat_max - obj_pos ) / vdiff;

            *tmin = std::min( time[0], time[1] );
            *tmax = std::max( time[0], time[1] );
        }
        else if ( 0.0f == tolerance_1 && 0.0f != tolerance_2 )
        {
            float time[2];

            // obj2 is the platform
            obj_pos  = src1_min;
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = ( obj_pos - plat_min ) / vdiff;
            time[1] = ( obj_pos - plat_max ) / vdiff;

            *tmin = std::min( time[0], time[1] );
            *tmax = std::max( time[0], time[1] );
        }
        else
        {
            // BOTH are platforms
            float time[4];

            // obj2 is the platform
            obj_pos  = src1_min;
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = ( obj_pos - plat_min ) / vdiff;
            time[1] = ( obj_pos - plat_max ) / vdiff;

            // obj1 is the platform
            obj_pos  = src2_min;
            plat_min = src1_min;
            plat_max = src1_max + tolerance_1;

            time[2] = ( plat_min - obj_pos ) / vdiff;
            time[3] = ( plat_max - obj_pos ) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
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
bool phys_intersect_oct_bb_close( const oct_bb_t * src1_orig, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t *  src2_orig, const fvec3_t& pos2, const fvec3_t& vel2, int test_platform, oct_bb_t * pdst, float *tmin, float *tmax )
{
    oct_bb_t src1, src2;
    oct_bb_t exp1, exp2;

    oct_bb_t intersection;

    oct_vec_t opos1, opos2;
    oct_vec_t ovel1, ovel2;

    int    cnt, index;
    bool found;
    float  tolerance;
    float  local_tmin, local_tmax;

    // handle optional parameters
    if ( NULL == tmin ) tmin = &local_tmin;
    if ( NULL == tmax ) tmax = &local_tmax;

    // do the objects interact at the very beginning of the update?
    if ( test_interaction_2( src1_orig, pos2, src2_orig, pos2, test_platform ) )
    {
        if ( NULL != pdst )
        {
            oct_bb_intersection( src1_orig, src2_orig, pdst );
        }

        return true;
    }

    // convert the position and velocity vectors to octagonal format
    oct_vec_ctor( ovel1, vel1 );
    oct_vec_ctor( opos1, pos1 );

    oct_vec_ctor( ovel2, vel2 );
    oct_vec_ctor( opos2, pos2 );

    oct_bb_add_ovec( src1_orig, opos1, &src1 );
    oct_bb_add_ovec( src2_orig, opos2, &src2 );

    // cycle through the coordinates to see when the two volumes might coincide
    found = false;
    *tmin = *tmax = -1.0f;
    for ( index = 0; index < OCT_COUNT; index ++ )
    {
        egolib_rv retval;
        float tmp_min, tmp_max;

        retval = phys_intersect_oct_bb_close_index( index, &src1, ovel1, &src2, ovel2, test_platform, &tmp_min, &tmp_max );
        if ( rv_fail == retval ) return false;

        if ( rv_success == retval )
        {
            if ( !found )
            {
                *tmin = tmp_min;
                *tmax = tmp_max;
                found = true;
            }
            else
            {
                *tmin = std::max( *tmin, tmp_min );
                *tmax = std::min( *tmax, tmp_max );
            }
        }

        if ( *tmax < *tmin ) return false;
    }

    // if the objects do not interact this frame let the caller know
    if ( *tmin > 1.0f || *tmax < 0.0f ) return false;

    // determine the expanded collision volumes for both objects
    phys_expand_oct_bb( &src1, vel1, *tmin, *tmax, &exp1 );
    phys_expand_oct_bb( &src2, vel2, *tmin, *tmax, &exp2 );

    // determine the intersection of these two volumes
    oct_bb_intersection( &exp1, &exp2, &intersection );

    // check to see if there is any possibility of interaction at all
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        if ( intersection.mins[cnt] > intersection.maxs[cnt] ) return false;
    }

    tolerance = ( 0 == test_platform ) ? 0.0f : PLATTOLERANCE;
    if ( intersection.mins[OCT_Z] > intersection.maxs[OCT_Z] + tolerance ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool phys_expand_oct_bb( const oct_bb_t * psrc, const fvec3_t& vel, const float tmin, const float tmax, oct_bb_t * pdst )
{
    oct_bb_t tmp_min, tmp_max;

    float abs_vel = fvec3_length_abs( vel );
    if ( 0.0f == abs_vel )
    {
        return oct_bb_copy( pdst, psrc ) ? true : false;
    }

    // determine the bounding volume at t == tmin
    if ( 0.0f == tmin )
    {
        oct_bb_copy( &tmp_min, psrc );
    }
    else
    {
        fvec3_t tmp_diff;

        tmp_diff.x = vel[kX] * tmin;
        tmp_diff.y = vel[kY] * tmin;
        tmp_diff.z = vel[kZ] * tmin;

        // adjust the bounding box to take in the position at the next step
        if ( !oct_bb_add_fvec3( psrc, tmp_diff, &tmp_min ) ) return false;
    }

    // determine the bounding volume at t == tmax
    if ( tmax == 0.0f )
    {
        oct_bb_copy( &tmp_max, psrc );
    }
    else
    {
        fvec3_t tmp_diff;

        tmp_diff.x = vel[kX] * tmax;
        tmp_diff.y = vel[kY] * tmax;
        tmp_diff.z = vel[kZ] * tmax;

        // adjust the bounding box to take in the position at the next step
        if ( !oct_bb_add_fvec3( psrc, tmp_diff, &tmp_max ) ) return false;
    }

    // determine bounding box for the range of times
    if ( !oct_bb_union( &tmp_min, &tmp_max, pdst ) ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool phys_expand_chr_bb( chr_t * pchr, float tmin, float tmax, oct_bb_t * pdst )
{


    oct_bb_t tmp_oct1, tmp_oct2;

    if ( !ACTIVE_PCHR( pchr ) ) return false;

    // copy the volume
    oct_bb_copy( &tmp_oct1, &( pchr->chr_max_cv ) );

    // add in the current position to the bounding volume
    oct_bb_add_fvec3( &( tmp_oct1 ), pchr->pos, &tmp_oct2 );

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb( &tmp_oct2, pchr->vel, tmin, tmax, pdst );
}

//--------------------------------------------------------------------------------------------
bool phys_expand_prt_bb( prt_t * pprt, float tmin, float tmax, oct_bb_t * pdst )
{
    /// @author BB
    /// @details use the object velocity to figure out where the volume that the particle will
    ///               occupy during this update

    oct_bb_t tmp_oct1, tmp_oct2;

    if ( !ACTIVE_PPRT( pprt ) ) return false;

    // copy the volume
    oct_bb_copy( &tmp_oct1, &( pprt->prt_max_cv ) );

    // add in the current position to the bounding volume
    oct_bb_add_fvec3( &tmp_oct1, pprt->pos, &tmp_oct2 );

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb( &tmp_oct2, pprt->vel, tmin, tmax, pdst );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_init_chr( breadcrumb_t * bc, chr_t * pchr )
{
    if ( NULL == bc ) return bc;

    BLANK_STRUCT_PTR( bc )
    bc->time = update_wld;

    if ( NULL == pchr ) return bc;

    bc->bits   = pchr->stoppedby;
    bc->radius = pchr->bump_1.size;
    bc->pos.x  = ( FLOOR( pchr->pos.x / GRID_FSIZE ) + 0.5f ) * GRID_FSIZE;
    bc->pos.y  = ( FLOOR( pchr->pos.y / GRID_FSIZE ) + 0.5f ) * GRID_FSIZE;
    bc->pos.z  = pchr->pos.z;

    bc->grid   = ego_mesh_get_grid( PMesh, bc->pos.x, bc->pos.y );
    bc->valid  = ( 0 == ego_mesh_test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) );

    bc->id = breadcrumb_guid++;

    return bc;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * breadcrumb_init_prt( breadcrumb_t * bc, prt_t * pprt )
{
    BIT_FIELD bits = 0;
    pip_t * ppip;

    if ( NULL == bc ) return bc;

    BLANK_STRUCT_PTR( bc )
    bc->time = update_wld;

    if ( NULL == pprt ) return bc;

    ppip = prt_get_ppip( GET_REF_PPRT( pprt ) );
    if ( NULL == ppip ) return bc;

    bits = MAPFX_IMPASS;
    if ( 0 != ppip->bump_money ) SET_BIT( bits, MAPFX_WALL );

    bc->bits   = bits;
    bc->radius = pprt->bump_real.size;

    prt_get_pos( pprt, bc->pos.v );
    bc->pos.x  = ( FLOOR( bc->pos.x / GRID_FSIZE ) + 0.5f ) * GRID_FSIZE;
    bc->pos.y  = ( FLOOR( bc->pos.y / GRID_FSIZE ) + 0.5f ) * GRID_FSIZE;

    bc->grid   = ego_mesh_get_grid( PMesh, bc->pos.x, bc->pos.y );
    bc->valid  = ( 0 == ego_mesh_test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) );

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
bool breadcrumb_list_full( const breadcrumb_list_t *  lst )
{
    if ( NULL == lst || !lst->on ) return true;

    return ( lst->count >= MAX_BREADCRUMB );
}

//--------------------------------------------------------------------------------------------
bool breadcrumb_list_empty( const breadcrumb_list_t * lst )
{
    if ( NULL == lst || !lst->on ) return true;

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
            if ( 0 != ego_mesh_test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) )
            {
                bc->valid = false;
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
bool breadcrumb_list_add( breadcrumb_list_t * lst, breadcrumb_t * pnew )
{
    int cnt, invalid_cnt;

    bool retval;
    breadcrumb_t * pold, *ptmp;

    if ( NULL == lst || !lst->on ) return false;

    if ( NULL == pnew ) return false;

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
                if ( ABS( ptmp->pos.x - pnew->pos.x ) < GRID_FSIZE && ABS( ptmp->pos.y - pnew->pos.y ) < GRID_FSIZE )
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
    retval = false;
    if ( NULL != pold )
    {
        *pold = *pnew;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_clear( phys_data_t * pphys )
{
    if ( NULL == pphys ) return pphys;

    apos_self_clear( &( pphys->aplat ) );
    apos_self_clear( &( pphys->acoll ) );
    fvec3_self_clear( pphys->avel.v );

    return pphys;
}

//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_ctor( phys_data_t * pphys )
{
    if ( NULL == pphys ) return pphys;

    phys_data_clear( pphys );

    pphys->bumpdampen = 1.0f;
    pphys->weight     = 1.0f;
    pphys->dampen     = 0.5f;

    return pphys;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool apos_self_union( apos_t * lhs, apos_t * rhs )
{
    int cnt;

    if ( NULL == lhs || NULL == rhs ) return false;

    // scan through the components of the vector and find the
    // maximum displacement
    for ( cnt = 0; cnt < 3; cnt ++ )
    {
        lhs->mins.v[cnt] = std::min( lhs->mins.v[cnt], rhs->mins.v[cnt] );
        lhs->maxs.v[cnt] = std::max( lhs->maxs.v[cnt], rhs->maxs.v[cnt] );
        lhs->sum.v[cnt] += rhs->sum.v[cnt];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool apos_self_union_fvec3( apos_t * lhs, const fvec3_base_t rhs )
{
    int cnt;

    if ( NULL == lhs ) return false;
    if ( NULL == rhs ) return true;

    LOG_NAN_FVEC3( rhs );

    // scan through the components of the vector and find the
    // maximum displacement
    for ( cnt = 0; cnt < 3; cnt ++ )
    {
        // find the extrema of the displacement
        if ( rhs[cnt] > 0.0f )
        {
            lhs->maxs.v[cnt] = std::max( lhs->maxs.v[cnt], rhs[cnt] );
        }
        else if ( rhs[cnt] < 0.0f )
        {
            lhs->mins.v[cnt] = std::min( lhs->mins.v[cnt], rhs[cnt] );
        }

        // find the sum of the displacement
        lhs->sum.v[cnt] += rhs[cnt];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool apos_self_union_index( apos_t * lhs, const float val, const int index )
{
    // find the maximum displacement at the given index

    if ( NULL == lhs ) return false;

    if ( index < 0 || index > 2 ) return false;

    LOG_NAN( val );

    // find the extrema of the displacement
    if ( val > 0.0f )
    {
        lhs->maxs.v[index] = std::max( lhs->maxs.v[index], val );
    }
    else if ( val < 0.0f )
    {
        lhs->mins.v[index] = std::min( lhs->mins.v[index], val );
    }

    // find the sum of the displacement
    lhs->sum.v[index] += val;

    return true;
}

//--------------------------------------------------------------------------------------------
bool apos_evaluate( const apos_t * src, fvec3_base_t dst )
{
    int cnt;

    if ( NULL == dst ) return false;

    if ( NULL == src )
    {
        return fvec3_self_clear( dst );
    }

    for ( cnt = 0; cnt < 3; cnt ++ )
    {
        dst[cnt] = src->maxs.v[cnt] + src->mins.v[cnt];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_sum_aplat( phys_data_t * pphys, const fvec3_base_t vec )
{
    if ( NULL == pphys ) return pphys;

    LOG_NAN_FVEC3( vec );

    apos_self_union_fvec3( &( pphys->aplat ), vec );

    return pphys;
}

//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_sum_acoll( phys_data_t * pphys, const fvec3_base_t vec )
{
    if ( NULL == pphys ) return pphys;

    LOG_NAN_FVEC3( vec );

    apos_self_union_fvec3( &( pphys->acoll ), vec );

    return pphys;
}

//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_sum_avel( phys_data_t * pphys, const fvec3_base_t vec )
{
    if ( NULL == pphys ) return pphys;

    LOG_NAN_FVEC3( vec );

    fvec3_self_sum( pphys->avel.v, vec );

    return pphys;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_sum_aplat_index( phys_data_t * pphys, const float val, const int index )
{
    if ( NULL == pphys ) return pphys;

    LOG_NAN( val );

    apos_self_union_index( &( pphys->aplat ), val, index );

    return pphys;
}

//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_sum_acoll_index( phys_data_t * pphys, const float val, const int index )
{
    if ( NULL == pphys ) return pphys;

    LOG_NAN( val );

    apos_self_union_index( &( pphys->acoll ), val, index );

    return pphys;
}

//--------------------------------------------------------------------------------------------
phys_data_t * phys_data_sum_avel_index( phys_data_t * pphys, const float val, const int index )
{
    if ( NULL == pphys ) return pphys;

    if ( index < 0 || index > 2 ) return pphys;

    LOG_NAN( val );

    pphys->avel.v[index] += val;

    return pphys;
}

//--------------------------------------------------------------------------------------------
//Inline below
//--------------------------------------------------------------------------------------------
bool test_interaction_close_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform )
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_close_2( &cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool test_interaction_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform )
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_2( &cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool test_interaction_close_1( const oct_bb_t * cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform )
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_close_2( cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool test_interaction_1( const oct_bb_t * cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform )
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_2( cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
bool test_interaction_close_2( const oct_bb_t * cv_a, const fvec3_t& pos_a, const oct_bb_t * cv_b, const fvec3_t& pos_b, int test_platform )
{
    int cnt;
    float depth;
    oct_vec_t oa, ob;

    if ( NULL == cv_a || NULL == cv_b ) return false;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        float ftmp1 = std::min(( ob[cnt] + cv_b->maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b->mins[cnt] ) );
        float ftmp2 = std::min(( oa[cnt] + cv_a->maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a->mins[cnt] ) );
        depth = std::max( ftmp1, ftmp2 );
        if ( depth <= 0.0f ) return false;
    }

    // treat the z coordinate the same as always
    depth = std::min( cv_b->maxs[OCT_Z] + ob[OCT_Z], cv_a->maxs[OCT_Z] + oa[OCT_Z] ) -
            std::max( cv_b->mins[OCT_Z] + ob[OCT_Z], cv_a->mins[OCT_Z] + oa[OCT_Z] );

    return TO_C_BOOL( test_platform ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f ) );
}

//--------------------------------------------------------------------------------------------
bool test_interaction_2( const oct_bb_t * cv_a, const fvec3_t& pos_a, const oct_bb_t * cv_b, const fvec3_t& pos_b, int test_platform )
{


    int cnt;
    oct_vec_t oa, ob;
    float depth;

    if ( NULL == cv_a || NULL == cv_b ) return false;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        depth  = std::min( cv_b->maxs[cnt] + ob[cnt], cv_a->maxs[cnt] + oa[cnt] ) -
                 std::max( cv_b->mins[cnt] + ob[cnt], cv_a->mins[cnt] + oa[cnt] );

        if ( depth <= 0.0f ) return false;
    }

    // treat the z coordinate the same as always
    depth = std::min( cv_b->maxs[OCT_Z] + ob[OCT_Z], cv_a->maxs[OCT_Z] + oa[OCT_Z] ) -
            std::max( cv_b->mins[OCT_Z] + ob[OCT_Z], cv_a->mins[OCT_Z] + oa[OCT_Z] );

    return TO_C_BOOL(( 0 != test_platform ) ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool get_depth_close_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth )
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    // shift the bumpers
    oct_bb_self_add_fvec3( &cv_a, pos_a );
    oct_bb_self_add_fvec3( &cv_b, pos_b );

    return get_depth_close_2( &cv_a, &cv_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool get_depth_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth )
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return get_depth_2( &cv_a, pos_a, &cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool get_depth_close_1( const oct_bb_t * cv_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth )
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    // shift the bumper
    oct_bb_self_add_fvec3( &cv_b, pos_b );

    return get_depth_close_2( cv_a, &cv_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool get_depth_1( const oct_bb_t * cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth )
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return get_depth_2( cv_a, pos_a, &cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
bool get_depth_close_2( const oct_bb_t * cv_a, const oct_bb_t * cv_b, bool break_out, oct_vec_t depth )
{
    int cnt;
    bool valid;
    float ftmp1, ftmp2;
    float opos_a, opos_b;

    if ( NULL == depth ) return false;

    if ( NULL == cv_a || NULL == cv_b ) return false;

    // calculate the depth
    valid = true;
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        // get positions from the bounding volumes
        opos_a = ( cv_a->mins[cnt] + cv_a->maxs[cnt] ) * 0.5f;
        opos_b = ( cv_b->mins[cnt] + cv_b->maxs[cnt] ) * 0.5f;

        // measue the depth
        ftmp1 = std::min( cv_b->maxs[cnt] - opos_a, opos_a - cv_b->mins[cnt] );
        ftmp2 = std::min( cv_a->maxs[cnt] - opos_b, opos_b - cv_a->mins[cnt] );
        depth[cnt] = std::max( ftmp1, ftmp2 );

        if ( depth[cnt] <= 0.0f )
        {
            valid = false;
            if ( break_out ) return false;
        }
    }

    // treat the z coordinate the same as always
    depth[OCT_Z]  = std::min( cv_b->maxs[OCT_Z], cv_a->maxs[OCT_Z] ) -
                    std::max( cv_b->mins[OCT_Z], cv_a->mins[OCT_Z] );

    if ( depth[OCT_Z] <= 0.0f )
    {
        valid = false;
        if ( break_out ) return false;
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
bool get_depth_2( const oct_bb_t * cv_a, const fvec3_t& pos_a, const oct_bb_t * cv_b, const fvec3_t& pos_b, bool break_out, oct_vec_t depth )
{
    int cnt;
    oct_vec_t oa, ob;
    bool valid;

    if ( NULL == cv_a || NULL == cv_b) return false;

    if ( NULL == depth ) return false;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    valid = true;
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        depth[cnt]  = std::min( cv_b->maxs[cnt] + ob[cnt], cv_a->maxs[cnt] + oa[cnt] ) -
                      std::max( cv_b->mins[cnt] + ob[cnt], cv_a->mins[cnt] + oa[cnt] );

        if ( depth[cnt] <= 0.0f )
        {
            valid = false;
            if ( break_out ) return false;
        }
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
apos_t * apos_self_clear( apos_t * val )
{
    if ( NULL == val ) return val;

    BLANK_STRUCT_PTR( val )

    return val;
}
