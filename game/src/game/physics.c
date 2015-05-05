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
#include "game/mesh.h"
#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Physics::Environment Physics::g_environment;

const float PLATFORM_STICKINESS =  0.01f;


static Ego::GUID breadcrumb_guid = 0;

static egolib_rv phys_intersect_oct_bb_index(int index, const oct_bb_t& src1, const oct_vec_v2_t& ovel1, const oct_bb_t& src2, const oct_vec_v2_t& ovel2, int test_platform, float *tmin, float *tmax);
static egolib_rv phys_intersect_oct_bb_close_index(int index, const oct_bb_t& src1, const oct_vec_v2_t& ovel1, const oct_bb_t& src2, const oct_vec_v2_t& ovel2, int test_platform, float *tmin, float *tmax);

/// @brief A test to determine whether two "fast moving" objects are interacting within a frame.
///        Designed to determine whether a bullet particle will interact with character.
static bool phys_intersect_oct_bb_close(const oct_bb_t& src1_orig, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t& src2_orig, const fvec3_t& pos2, const fvec3_t& vel2, int test_platform, oct_bb_t& dst, float *tmin, float *tmax);
static bool phys_estimate_depth(const oct_vec_v2_t& odepth, const float exponent, fvec3_t& nrm, float& depth);
static float phys_get_depth(const oct_vec_v2_t& odepth, const fvec3_t& nrm);
static bool phys_warp_normal(const float exponent, fvec3_t& nrm);
static bool phys_get_pressure_depth(const oct_bb_t& bb_a, const oct_bb_t& bb_b, oct_vec_v2_t& odepth);
static bool phys_get_collision_depth(const oct_bb_t& bb_a, const oct_bb_t& bb_b, oct_vec_v2_t& odepth);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool phys_get_collision_depth(const oct_bb_t& bb_a, const oct_bb_t& bb_b, oct_vec_v2_t& odepth)
{
    odepth.setZero();

    // are the initial volumes any good?
    if (bb_a.empty || bb_b.empty) return false;

    // is there any overlap?
    oct_bb_t otmp;
    if (rv_success != oct_bb_intersection(&bb_a, &bb_b, &otmp))
    {
        return false;
    }

    // Estimate the "cm position" of the objects by the bounding volumes.
    oct_vec_v2_t opos_a = bb_a.getMid();
    oct_vec_v2_t opos_b = bb_b.getMid();

    // find the (signed) depth in each dimension
    bool retval = true;
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        float fdiff = opos_b[i] - opos_a[i];
        float fdepth = otmp.maxs[i] - otmp.mins[i];

        // if the measured depth is less than zero, or the difference in positions
        // is ambiguous, this algorithm fails
        if (fdepth <= 0.0f || 0.0f == fdiff) retval = false;

        odepth[i] = (fdiff < 0.0f) ? -fdepth : fdepth;
    }
    odepth[OCT_XY] *= Ego::Math::invSqrtTwo<float>();
    odepth[OCT_YX] *= Ego::Math::invSqrtTwo<float>();

    return retval;
}

//--------------------------------------------------------------------------------------------
bool phys_get_pressure_depth(const oct_bb_t& bb_a, const oct_bb_t& bb_b, oct_vec_v2_t& odepth)
{
    odepth.setZero();

    // assume the best
    bool result = true;

    // scan through the dimensions of the oct_bbs
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        float diff1 = bb_a.maxs[i] - bb_b.mins[i];
        float diff2 = bb_b.maxs[i] - bb_a.mins[i];

        if (diff1 < 0.0f || diff2 < 0.0f)
        {
            // this case will only happen if there is no overlap in one of the dimensions,
            // meaning there was a bad collision detection... it should NEVER happen.
            // In any case this math still generates the proper direction for
            // the normal pointing away from b.
            if (std::abs(diff1) < std::abs(diff2))
            {
                odepth[i] = diff1;
            }
            else
            {
                odepth[i] = -diff2;
            }

            result = false;
        }
        else if (diff1 < diff2)
        {
            odepth[i] = -diff1;
        }
        else
        {
            odepth[i] = diff2;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool phys_warp_normal(const float exponent, fvec3_t& nrm)
{
    // use the exponent to warp the normal into a cylinder-like shape, if needed

    if (1.0f == exponent) return true;

    if (0.0f == nrm.length_abs()) return false;

    float length_hrz_2 = fvec2_t(nrm[kX],nrm[kY]).length_2();
    float length_vrt_2 = nrm.length_2() - length_hrz_2;

    nrm[kX] = nrm[kX] * POW( length_hrz_2, 0.5f * ( exponent - 1.0f ) );
    nrm[kY] = nrm[kY] * POW( length_hrz_2, 0.5f * ( exponent - 1.0f ) );
    nrm[kZ] = nrm[kZ] * POW( length_vrt_2, 0.5f * ( exponent - 1.0f ) );

    // normalize the normal
	nrm.normalize();
    return nrm.length() >= 0.0f;
}

//--------------------------------------------------------------------------------------------
float phys_get_depth(const oct_vec_v2_t& podepth, const fvec3_t& nrm)
{
    const float max_val = 1e6;

    if (0.0f == nrm.length_abs()) return max_val;

    // Convert the normal vector into an octagonal normal vector.
    oct_vec_v2_t onrm(nrm);
    onrm[OCT_XY] *= Ego::Math::invSqrtTwo<float>();
    onrm[OCT_YX] *= Ego::Math::invSqrtTwo<float>();

    // Calculate the minimum depth in each dimension.
    float depth = max_val;
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        if (0.0f == podepth[i])
        {
            depth = 0.0f;
            break;
        }

        if (0.0f != onrm[i])
        {
            float ftmp = podepth[i] / onrm[i];
            if (ftmp <= 0.0f)
            {
                depth = 0.0f;
                break;
            }

            depth = std::min(depth, ftmp);
        }
    }

    return depth;
}

//--------------------------------------------------------------------------------------------
bool phys_estimate_depth(const oct_vec_v2_t& odepth, const float exponent, fvec3_t& nrm, float& depth)
{
    // use the given (signed) podepth info to make a normal vector, and measure
    // the shortest distance to the border

    fvec3_t nrm_aa;

    if (0.0f != odepth[OCT_X]) nrm_aa.x = 1.0f / odepth[OCT_X];
    if (0.0f != odepth[OCT_Y]) nrm_aa.y = 1.0f / odepth[OCT_Y];
    if (0.0f != odepth[OCT_Z]) nrm_aa.z = 1.0f / odepth[OCT_Z];

    if ( 1.0f == exponent )
    {
        nrm_aa.normalize(); /// @todo Divide-by-zero ...
    }
    else
    {
        phys_warp_normal(exponent, nrm_aa);
    }

    // find a minimum distance
    float tmin_aa = 1e6;

    if (nrm_aa.x != 0.0f)
    {
        float ftmp = odepth[OCT_X] / nrm_aa.x;
        ftmp = std::max(0.0f, ftmp);
        tmin_aa = std::min(tmin_aa, ftmp);
    }

    if (nrm_aa.y != 0.0f)
    {
        float ftmp = odepth[OCT_Y] / nrm_aa.y;
        ftmp = std::max(0.0f, ftmp);
        tmin_aa = std::min(tmin_aa, ftmp);
    }

    if (nrm_aa.z != 0.0f)
    {
        float ftmp = odepth[OCT_Z] / nrm_aa.z;
        ftmp = std::max(0.0f, ftmp);
        tmin_aa = std::min(tmin_aa, ftmp);
    }

    if (tmin_aa <= 0.0f || tmin_aa >= 1e6) return false;

    // Next do the diagonal axes.
	fvec3_t nrm_diag = fvec3_t::zero();

    if (0.0f != odepth[OCT_XY]) nrm_diag.x = 1.0f / (odepth[OCT_XY] * Ego::Math::invSqrtTwo<float>());
    if (0.0f != odepth[OCT_YX]) nrm_diag.y = 1.0f / (odepth[OCT_YX] * Ego::Math::invSqrtTwo<float>());
    if (0.0f != odepth[OCT_Z ]) nrm_diag.z = 1.0f / odepth[OCT_Z];

    if (1.0f == exponent)
    {
        nrm_diag.normalize();
    }
    else
    {
        phys_warp_normal(exponent, nrm_diag);
    }

    // find a minimum distance
    float tmin_diag = 1e6;

    if (nrm_diag.x != 0.0f)
    {
        float ftmp = Ego::Math::invSqrtTwo<float>() * odepth[OCT_XY] / nrm_diag.x;
        ftmp = std::max(0.0f, ftmp);
        tmin_diag = std::min(tmin_diag, ftmp);
    }

    if (nrm_diag.y != 0.0f)
    {
        float ftmp = Ego::Math::invSqrtTwo<float>() * odepth[OCT_YX] / nrm_diag.y;
        ftmp = std::max(0.0f, ftmp);
        tmin_diag = std::min(tmin_diag, ftmp);
    }

    if (nrm_diag.z != 0.0f)
    {
        float ftmp = odepth[OCT_Z] / nrm_diag.z;
        ftmp = std::max(0.0f, ftmp);
        tmin_diag = std::min(tmin_diag, ftmp);
    }

    if (tmin_diag <= 0.0f || tmin_diag >= 1e6) return false;

    float tmin;
    if (tmin_aa < tmin_diag)
    {
        tmin = tmin_aa;
		nrm = nrm_aa;
    }
    else
    {
        tmin = tmin_diag;

        // !!!! rotate the diagonal axes onto the axis aligned ones !!!!!
        nrm[kX] = (nrm_diag.x - nrm_diag.y) * Ego::Math::invSqrtTwo<float>();
        nrm[kY] = (nrm_diag.x + nrm_diag.y) * Ego::Math::invSqrtTwo<float>();
        nrm[kZ] = nrm_diag.z;
    }

    // normalize this normal
	nrm.normalize();
    bool result = nrm.length() > 0.0f;

    // find the depth in the direction of the normal, if possible
    if (result)
    {
        depth = tmin;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool phys_estimate_collision_normal(const oct_bb_t& obb_a, const oct_bb_t& obb_b, const float exponent, oct_vec_v2_t& odepth, fvec3_t& nrm, float& depth)
{
    // estimate the normal for collision volumes that are partially overlapping

#if 0
    // is everything valid?
    if (NULL == obb_a || NULL == obb_b) return false;
#endif
    // Do we need to use the more expensive algorithm?
    bool use_pressure = false;
    if (oct_bb_t::contains(&obb_a, &obb_b))
    {
        use_pressure = true;
    }
    else if (oct_bb_t::contains(&obb_b, &obb_a))
    {
        use_pressure = true;
    }

    if (!use_pressure)
    {
        // Try to get the collision depth.
        if (!phys_get_collision_depth(obb_a, obb_b, odepth))
        {
            use_pressure = true;
        }
    }

    if (use_pressure)
    {
        return phys_estimate_pressure_normal(obb_a, obb_b, exponent, odepth, nrm, depth);
    }

    return phys_estimate_depth(odepth, exponent, nrm, depth);
}

//--------------------------------------------------------------------------------------------
bool phys_estimate_pressure_normal(const oct_bb_t& obb_a, const oct_bb_t& obb_b, const float exponent, oct_vec_v2_t& odepth, fvec3_t& nrm, float& depth)
{
    // use a more robust algorithm to get the normal no matter how the 2 volumes are
    // related
#if 0
    float loc_tmin;
    // handle "optional" parameters
    if ( NULL == depth ) depth = &loc_tmin;
    if ( NULL == obb_a || NULL == obb_b ) return false;
#endif
    // calculate the direction of the nearest way out for each octagonal axis
    if (!phys_get_pressure_depth(obb_a, obb_b, odepth))
    {
        return false;
    }

    return phys_estimate_depth(odepth, exponent, nrm, depth);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv phys_intersect_oct_bb_index(int index, const oct_bb_t& src1, const oct_vec_v2_t& ovel1, const oct_bb_t& src2, const oct_vec_v2_t& ovel2, int test_platform, float *tmin, float *tmax)
{
    if (!tmin)
    {
        throw std::invalid_argument("nullptr == tmin");
    }
    if (!tmax)
    {
        throw std::invalid_argument("nullptr == tmax");
    }
    if (index < 0)
    {
        throw std::invalid_argument("index < 0");
    }
    if (index >= OCT_COUNT)
    {
        throw std::invalid_argument("index >= OCT_COUNT");
    }
#if 0
    if (!tmin || !tmax) return rv_error;
    if (index < 0 || index >= OCT_COUNT) return rv_error;
#endif

    float vdiff = ovel2[index] - ovel1[index];
    if ( 0.0f == vdiff ) return rv_fail;

    float src1_min = src1.mins[index];
    float src1_max = src1.maxs[index];
    float src2_min = src2.mins[index];
    float src2_max = src2.maxs[index];

    if (OCT_Z != index)
    {
        // Is there any possibility of the 2 objects acting as a platform pair.
        bool close_test_1 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ1);
        bool close_test_2 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ2);

        // Only do a close test if the object's feet are above the platform.
        close_test_1 = close_test_1 && (src1.mins[OCT_Z] > src2.maxs[OCT_Z]);
        close_test_2 = close_test_2 && (src2.mins[OCT_Z] > src1.maxs[OCT_Z]);

        if (!close_test_1 && !close_test_2)
        {
            // NEITHER is a platform.
            float time[4];

            time[0] = (src1_min - src2_min) / vdiff;
            time[1] = (src1_min - src2_max) / vdiff;
            time[2] = (src1_max - src2_min) / vdiff;
            time[3] = (src1_max - src2_max) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
        else
        {
            return phys_intersect_oct_bb_close_index(index, src1, ovel1, src2, ovel2, test_platform, tmin, tmax);
        }
    }
    else /* OCT_Z == index */
    {
        float plat_min, plat_max;

        // Add in a tolerance into the vertical direction for platforms
        float tolerance_1 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ1) ? PLATTOLERANCE : 0.0f;
        float tolerance_2 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ2) ? PLATTOLERANCE : 0.0f;

        if ( 0.0f == tolerance_1 && 0.0f == tolerance_2 )
        {
            // NEITHER is a platform.
            float time[4];

            time[0] = (src1_min - src2_min) / vdiff;
            time[1] = (src1_min - src2_max) / vdiff;
            time[2] = (src1_max - src2_min) / vdiff;
            time[3] = (src1_max - src2_max) / vdiff;

            *tmin = std::min( std::min( time[0], time[1] ), std::min( time[2], time[3] ) );
            *tmax = std::max( std::max( time[0], time[1] ), std::max( time[2], time[3] ) );
        }
        else if (0.0f == tolerance_1)
        {
            float time[4];

            // 2nd object is a platform.
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = (src1_min - plat_min) / vdiff;
            time[1] = (src1_min - plat_max) / vdiff;
            time[2] = (src1_max - plat_min) / vdiff;
            time[3] = (src1_max - plat_max) / vdiff;

            *tmin = std::min(std::min(time[0], time[1]), std::min(time[2], time[3]));
            *tmax = std::max(std::max(time[0], time[1]), std::max(time[2], time[3]));
        }
        else if (0.0f == tolerance_2)
        {
            float time[4];

            // 1st object is a platform.
            plat_min = src1_min;
            plat_max = src1_max + tolerance_2;

            time[0] = (plat_min - src2_min) / vdiff;
            time[1] = (plat_min - src2_max) / vdiff;
            time[2] = (plat_max - src2_min) / vdiff;
            time[3] = (plat_max - src2_max) / vdiff;

            *tmin = std::min(std::min(time[0], time[1]), std::min(time[2], time[3]));
            *tmax = std::max(std::max(time[0], time[1]), std::max(time[2], time[3]));
        }
        else if ( tolerance_1 > 0.0f && tolerance_2 > 0.0f )
        {
            // BOTH are platforms.
            // They cannot both act as plaforms at the same time,
            // so do 8 tests.

            float time[8];
            float tmp_min1, tmp_max1;
            float tmp_min2, tmp_max2;

            // Assume: 2nd object is platform.
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = (src1_min - plat_min) / vdiff;
            time[1] = (src1_min - plat_max) / vdiff;
            time[2] = (src1_max - plat_min) / vdiff;
            time[3] = (src1_max - plat_max) / vdiff;
            tmp_min1 = std::min(std::min(time[0], time[1]), std::min(time[2], time[3]));
            tmp_max1 = std::max(std::max(time[0], time[1]), std::max(time[2], time[3]));

            // Assume: 1st object is platform.
            plat_min = src1_min;
            plat_max = src1_max + tolerance_2;

            time[4] = (plat_min - src2_min) / vdiff;
            time[5] = (plat_min - src2_max) / vdiff;
            time[6] = (plat_max - src2_min) / vdiff;
            time[7] = (plat_max - src2_max) / vdiff;
            tmp_min2 = std::min(std::min(time[4], time[5]), std::min(time[6], time[7]));
            tmp_max2 = std::max(std::max(time[4], time[5]), std::max(time[6], time[7]));

            *tmin = std::min(tmp_min1, tmp_min2);
            *tmax = std::max(tmp_max1, tmp_max2);
        }
    }

    // Normalize the results for the diagonal directions.
    if (OCT_XY == index || OCT_YX == index)
    {
        *tmin *= Ego::Math::invSqrtTwo<float>();
        *tmax *= Ego::Math::invSqrtTwo<float>();
    }

    if (*tmax <= *tmin) return rv_fail;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool phys_intersect_oct_bb(const oct_bb_t& src1_orig, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t& src2_orig, const fvec3_t& pos2, const fvec3_t& vel2, int test_platform, oct_bb_t& dst, float *tmin, float *tmax)
{
    /// @author BB
	/// @details A test to determine whether two "fast moving" objects are interacting within a frame.
	///               Designed to determine whether a bullet particle will interact with character.

    float  local_tmin, local_tmax;

    // handle optional parameters
    if ( NULL == tmin ) tmin = &local_tmin;
    if ( NULL == tmax ) tmax = &local_tmax;

    // convert the position and velocity vectors to octagonal format
    oct_vec_v2_t opos1(pos1), opos2(pos2),
                 ovel1(vel1), ovel2(vel2);

    oct_bb_t src1, src2;

    // shift the bounding boxes to their starting positions
    oct_bb_translate(&src1_orig, opos1, &src1);
    oct_bb_translate(&src2_orig, opos2, &src2);

    bool found = false;
    *tmin = +1.0e6;
    *tmax = -1.0e6;

    int failure_count = 0;
    bool failure[OCT_COUNT];
    if (fvec3_dist_abs(vel1, vel2) < 1.0e-6)
    {
        // No relative motion, so avoid the loop to save time.
        failure_count = OCT_COUNT;
    }
    else
    {
        // Cycle through the coordinates to see when the two volumes might coincide.
        for (size_t index = 0; index < OCT_COUNT; ++index)
        {
            egolib_rv retval;

            if (std::abs(ovel1[index] - ovel2[index]) < 1.0e-6)
            {
                failure[index] = true;
                failure_count++;
            }
            else
            {
                float tmp_min = 0.0f, tmp_max = 0.0f;

                retval = phys_intersect_oct_bb_index(index, src1, ovel1, src2, ovel2, test_platform, &tmp_min, &tmp_max);

                // check for overflow
                if (float_bad(tmp_min) || float_bad(tmp_max))
                {
                    retval = rv_fail;
                }

                if (rv_fail == retval)
                {
                    // This case will only occur if the objects are not moving relative to each other.
                    failure[index] = true;
                    failure_count++;
                }
                else if (rv_success == retval)
                {
                    failure[index] = false;

                    if (!found)
                    {
                        *tmin = tmp_min;
                        *tmax = tmp_max;
                        found = true;
                    }
                    else
                    {
                        *tmin = std::max(*tmin, tmp_min);
                        *tmax = std::min(*tmax, tmp_max);
                    }

                    // Check the values vs. reasonable bounds.
                    if (*tmax <= *tmin) return false;
                    if (*tmin > 1.0f || *tmax < 0.0f) return false;
                }
            }
        }
    }

    if (OCT_COUNT == failure_count)
    {
        // No relative motion on any axis.
        // Just say that they are interacting for the whole frame.

        *tmin = 0.0f;
        *tmax = 1.0f;

        // Determine the intersection of these two expanded volumes (for this frame).
        oct_bb_intersection(&src1, &src2, &dst);
    }
    else
    {
        float tmp_min, tmp_max;

        // Check to see if there the intersection times make any sense.
        if (*tmax <= *tmin) return false;

        // Check whether there is any overlap this frame.
        if (*tmin >= 1.0f || *tmax <= 0.0f) return false;

        // Clip the interaction time to just one frame.
        tmp_min = CLIP(*tmin, 0.0f, 1.0f);
        tmp_max = CLIP(*tmax, 0.0f, 1.0f);

        // determine the expanded collision volumes for both objects (for this frame)
        oct_bb_t exp1, exp2;
        phys_expand_oct_bb(src1, vel1, tmp_min, tmp_max, exp1);
        phys_expand_oct_bb(src2, vel2, tmp_min, tmp_max, exp2);

        // determine the intersection of these two expanded volumes (for this frame)
        oct_bb_intersection(&exp1, &exp2, &dst);
    }

    if (0 != test_platform)
    {
        dst.maxs[OCT_Z] += PLATTOLERANCE;
        oct_bb_t::validate(&dst);
    }

    if (dst.empty) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv phys_intersect_oct_bb_close_index(int index, const oct_bb_t& src1, const oct_vec_v2_t& ovel1, const oct_bb_t& src2, const oct_vec_v2_t& ovel2, int test_platform, float *tmin, float *tmax)
{
    if (!tmin)
    {
        throw std::invalid_argument("nullptr == tmin");
    }
    if (!tmax)
    {
        throw std::invalid_argument("nullptr == tmax");
    }
    if (index < 0)
    {
        throw std::invalid_argument("index < 0");
    }
    if (index >= OCT_COUNT)
    {
        throw std::invalid_argument("index >= OCT_COUNT");
    }
    float vdiff = ovel2[index] - ovel1[index];
    if (0.0f == vdiff) return rv_fail;

    /// @todo Use src1.getMin(index), src2.getMax(index) and src1.getMid(index).
    float src1_min = src1.mins[index];
    float src1_max = src1.maxs[index];
    float opos1 = (src1_min + src1_max) * 0.5f;

    /// @todo Use src2.getMin(index), src2.getMax(index) and src2.getMid(index).
    float src2_min = src2.mins[index];
    float src2_max = src2.maxs[index];
    float opos2 = (src2_min + src2_max) * 0.5f;

    if (OCT_Z != index)
    {
        bool platform_1 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ1);
        bool platform_2 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ2);

        if (!platform_1 && !platform_2)
        {
            // NEITHER is a platform.
            // Use the eqn. from phys_intersect_oct_bb_index().

            float time[4];

            time[0] = (src1_min - src2_min) / vdiff;
            time[1] = (src1_min - src2_max) / vdiff;
            time[2] = (src1_max - src2_min) / vdiff;
            time[3] = (src1_max - src2_max) / vdiff;

			*tmin = std::min({ time[0], time[1], time[2], time[3] });
			*tmax = std::max({ time[0], time[1], time[2], time[3] });
        }
        else if ( platform_1 && !platform_2 )
        {
            float time[2];

            // 1st object is the platform.
            time[0] = (src1_min - opos2) / vdiff;
            time[1] = (src1_max - opos2) / vdiff;

            *tmin = std::min(time[0], time[1]);
            *tmax = std::max(time[0], time[1]);
        }
        else if (!platform_1 && platform_2)
        {
            float time[2];

            // 2nd object is the platform.
            time[0] = (opos1 - src2_min) / vdiff;
            time[1] = (opos1 - src2_max) / vdiff;

            *tmin = std::min(time[0], time[1]);
            *tmax = std::max(time[0], time[1]);
        }
        else
        {
            // BOTH are platforms. must check all possibilities.
            float time[4];

            // 1st object is the platform.
            time[0] = (src1_min - opos2) / vdiff;
            time[1] = (src1_max - opos2) / vdiff;

            // 2nd object 2 is the platform.
            time[2] = (opos1 - src2_min) / vdiff;
            time[3] = (opos1 - src2_max) / vdiff;

            *tmin = std::min(std::min(time[0], time[1]), std::min(time[2], time[3]));
            *tmax = std::max(std::max(time[0], time[1]), std::max(time[2], time[3]));
        }
    }
    else /* OCT_Z == index */
    {
        float plat_min, plat_max;
        float obj_pos;

        float tolerance_1 =  HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ1)
                          ? PLATTOLERANCE : 0.0f;
        float tolerance_2 =  HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ2)
                          ? PLATTOLERANCE : 0.0f;

        if (0.0f == tolerance_1 && 0.0f == tolerance_2)
        {
            // NEITHER is a platform.
            // Use the eqn. from phys_intersect_oct_bb_index().

            float time[4];

            time[0] = (src1_min - src2_min) / vdiff;
            time[1] = (src1_min - src2_max) / vdiff;
            time[2] = (src1_max - src2_min) / vdiff;
            time[3] = (src1_max - src2_max) / vdiff;

            *tmin = std::min({ time[0], time[1], time[2], time[3] });
            *tmax = std::max({ time[0], time[1], time[2], time[3] });
        }
        else if (0.0f != tolerance_1 && 0.0f == tolerance_2)
        {
            float time[2];

            // 1st object is the platform.
            obj_pos  = src2_min;
            plat_min = src1_min;
            plat_max = src1_max + tolerance_1;

            time[0] = (plat_min - obj_pos) / vdiff;
            time[1] = (plat_max - obj_pos) / vdiff;

            *tmin = std::min(time[0], time[1]);
            *tmax = std::max(time[0], time[1]);
        }
        else if (0.0f == tolerance_1 && 0.0f != tolerance_2)
        {
            float time[2];

            // 2nd object is the platform.
            obj_pos  = src1_min;
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = (obj_pos - plat_min) / vdiff;
            time[1] = (obj_pos - plat_max) / vdiff;

            *tmin = std::min(time[0], time[1]);
            *tmax = std::max(time[0], time[1]);
        }
        else
        {
            // BOTH are platforms.
            float time[4];

            // 2nd object is a platform.
            obj_pos  = src1_min;
            plat_min = src2_min;
            plat_max = src2_max + tolerance_2;

            time[0] = (obj_pos - plat_min) / vdiff;
            time[1] = (obj_pos - plat_max) / vdiff;

            // 1st object is a platform.
            obj_pos  = src2_min;
            plat_min = src1_min;
            plat_max = src1_max + tolerance_1;

            time[2] = (plat_min - obj_pos) / vdiff;
            time[3] = (plat_max - obj_pos) / vdiff;

            *tmin = std::min({ time[0], time[1], time[2], time[3] });
            *tmax = std::max({ time[0], time[1], time[2], time[3] });
        }
    }

    // Normalize the results for the diagonal directions.
    if (OCT_XY == index || OCT_YX == index)
    {
        *tmin *= Ego::Math::invSqrtTwo<float>();
        *tmax *= Ego::Math::invSqrtTwo<float>();
    }

    if (*tmax < *tmin) return rv_fail;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool phys_intersect_oct_bb_close(const oct_bb_t& src1_orig, const fvec3_t& pos1, const fvec3_t& vel1, const oct_bb_t& src2_orig, const fvec3_t& pos2, const fvec3_t& vel2, int test_platform, oct_bb_t& dst, float *tmin, float *tmax)
{
    if (!tmin)
    {
        throw std::invalid_argument("nullptr == tmin");
    }
    if (!tmax)
    {
        throw std::invalid_argument("nullptr == tmax");
    }

    // Do the objects interact at the very beginning of the update?
    if (test_interaction_2(src1_orig, pos2, src2_orig, pos2, test_platform))
    {
#if 0
        if (NULL != pdst )
#endif
        {
            oct_bb_intersection(&src1_orig, &src2_orig, &dst);
        }

        return true;
    }

    // convert the position and velocity vectors to octagonal format
    oct_vec_v2_t opos1(pos1), opos2(pos2),
                 ovel1(vel1), ovel2(vel2);

    oct_bb_t src1, src2;

    oct_bb_translate(&src1_orig, opos1, &src1);
    oct_bb_translate(&src2_orig, opos2, &src2);

    // Cycle through the coordinates to see when the two volumes might coincide.
    bool found = false;
    *tmin = *tmax = -1.0f;
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        egolib_rv retval;
        float tmp_min, tmp_max;

        retval = phys_intersect_oct_bb_close_index(i, src1, ovel1, src2, ovel2, test_platform, &tmp_min, &tmp_max);
        if (rv_fail == retval) return false;

        if (rv_success == retval)
        {
            if (!found)
            {
                *tmin = tmp_min;
                *tmax = tmp_max;
                found = true;
            }
            else
            {
                *tmin = std::max(*tmin, tmp_min);
                *tmax = std::min(*tmax, tmp_max);
            }
        }

        if (*tmax < *tmin) return false;
    }

    // If the objects do not interact this frame let the caller know.
    if (*tmin > 1.0f || *tmax < 0.0f) return false;

    // Determine the expanded collision volumes for both objects.
    oct_bb_t exp1, exp2;
    phys_expand_oct_bb(src1, vel1, *tmin, *tmax, exp1);
    phys_expand_oct_bb(src2, vel2, *tmin, *tmax, exp2);

    // determine the intersection of these two volumes
    oct_bb_t intersection;
    oct_bb_intersection(&exp1, &exp2, &intersection);

    // check to see if there is any possibility of interaction at all
    for (size_t i = 0; i < OCT_Z; ++i)
    {
        if (intersection.mins[i] > intersection.maxs[i]) return false;
    }

    float tolerance = (0 == test_platform) ? 0.0f : PLATTOLERANCE;
    if (intersection.mins[OCT_Z] > intersection.maxs[OCT_Z] + tolerance)
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool phys_expand_oct_bb(const oct_bb_t& src, const fvec3_t& vel, const float tmin, const float tmax, oct_bb_t& dst)
{
#if 0
    if (!psrc || !pdst)
    {
        return false;
    }
#endif

    if (0.0f == vel.length_abs())
    {
        dst = src;
        return true;
    }

    oct_bb_t tmp_min, tmp_max;
    // Determine the bounding volume at t == tmin.
    if (0.0f == tmin)
    {
        tmp_min = src;
    }
    else
    {
        fvec3_t tmp_diff = vel * tmin;
        // Adjust the bounding box to take in the position at the next step.
        if (!oct_bb_translate(&src, tmp_diff, &tmp_min)) return false;
    }

    // Determine the bounding volume at t == tmax.
    if ( tmax == 0.0f )
    {
        tmp_max = src;
    }
    else
    {
        fvec3_t tmp_diff = vel * tmax;
        // Adjust the bounding box to take in the position at the next step.
        if (!oct_bb_translate(&src, tmp_diff, &tmp_max)) return false;
    }

    // Determine bounding box for the range of times.
    if (!oct_bb_union(&tmp_min, &tmp_max, &dst)) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool phys_expand_chr_bb(Object *pchr, float tmin, float tmax, oct_bb_t& dst)
{
    if (!ACTIVE_PCHR(pchr)) return false;

    // copy the volume
    oct_bb_t tmp_oct1 = pchr->chr_max_cv;

    // add in the current position to the bounding volume
    oct_bb_t tmp_oct2;
    oct_bb_translate(&(tmp_oct1), pchr->getPosition(), &tmp_oct2);

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb(tmp_oct2, pchr->vel, tmin, tmax, dst);
}

//--------------------------------------------------------------------------------------------
bool phys_expand_prt_bb(prt_t *pprt, float tmin, float tmax, oct_bb_t& dst)
{
    /// @author BB
    /// @details use the object velocity to figure out where the volume that the particle will
    ///               occupy during this update

    if ( !ACTIVE_PPRT( pprt ) ) return false;

    // copy the volume
    oct_bb_t tmp_oct1 = pprt->prt_max_cv;

    // add in the current position to the bounding volume
    oct_bb_t tmp_oct2;
    oct_bb_translate( &tmp_oct1, pprt->pos, &tmp_oct2 );

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb(tmp_oct2, pprt->vel, tmin, tmax, dst);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  Snap a world coordinate point to grid.
 *  
 *  The point is moved along the x- and y-axis such that it is centered on the tile it is on.
 * @param p
 *  the point
 * @return
 *  the snapped world coordinate point
 */
static fvec3_t snap(const fvec3_t& p)
{
    return fvec3_t((FLOOR(p.x / GRID_FSIZE) + 0.5f) * GRID_FSIZE,
                   (FLOOR(p.y / GRID_FSIZE) + 0.5f) * GRID_FSIZE,
                   p.z);
}

breadcrumb_t *breadcrumb_t::init(breadcrumb_t *self, Object *chr)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!chr)
    {
        throw std::invalid_argument("nullptr == chr");
    }
    self->reset();
    self->time = update_wld;

    self->bits   = chr->stoppedby;
    self->radius = chr->bump_1.size;
    self->pos = snap(chr->getPosition());
    self->grid   = ego_mesh_t::get_grid(PMesh, PointWorld(self->pos.x, self->pos.y)).getI();
    self->valid  = (0 == ego_mesh_test_wall(PMesh, self->pos, self->radius, self->bits, nullptr));

    self->id = breadcrumb_guid++;

    return self;
}

breadcrumb_t *breadcrumb_t::init(breadcrumb_t *self, prt_t *particle)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!particle)
    {
        throw std::invalid_argument("nullptr == particle");
    }
    pip_t *profile = prt_get_ppip(GET_REF_PPRT(particle));
    if (!profile)
    {
        throw std::invalid_argument("nullptr == prpfile");
    }
    self->reset();
    self->time = update_wld;


    BIT_FIELD bits = MAPFX_IMPASS;
    if (0 != profile->bump_money) SET_BIT(bits, MAPFX_WALL);

    self->bits = bits;
    self->radius = particle->bump_real.size;

    fvec3_t pos = prt_t::get_pos_v_const(particle);
    self->pos = snap(prt_t::get_pos_v_const(particle));
    self->grid   = ego_mesh_t::get_grid(PMesh, PointWorld(self->pos.x, self->pos.y)).getI();
    self->valid  = ( 0 == ego_mesh_test_wall( PMesh, self->pos, self->radius, self->bits, nullptr));

    self->id = breadcrumb_guid++;

    return self;
}

bool breadcrumb_t::cmp(const breadcrumb_t& x, const breadcrumb_t& y)
{
    if (x.time < y.time)
    {
        return true;
    }
    else if (x.time > y.time)
    {
        return false;
    }
    if (x.id < y.id)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void breadcrumb_list_t::validate(breadcrumb_list_t *self)
{
    if (!self || !self->on) return;

    // Invalidate breadcrumbs which are not valid anymore.
	size_t invalid = 0;
    for (size_t i = 0; i < self->count; ++i)
    {
        breadcrumb_t& breadcrumb = self->lst[i];

        if (!breadcrumb.valid)
        {
            invalid++;
        }
        else
        {
            if (0 != ego_mesh_test_wall(PMesh, breadcrumb.pos, breadcrumb.radius, breadcrumb.bits, nullptr))
            {
                breadcrumb.valid = false;
                invalid++;
            }
        }
    }

    // If there were invalid breadcrumbs ...
    if (invalid > 0)
    {
        // ... compact the list.
        self->compact();
    }

    // Sort the values from lowest to highest.
    if (self->count > 1)
    {
        std::sort(self->lst.begin(),self->lst.begin() + self->count,breadcrumb_t::cmp);
    }
}

breadcrumb_t *breadcrumb_list_t::last_valid(breadcrumb_list_t *self)
{
    if (!self || !self->on)
    {
        return nullptr;
    }

    breadcrumb_list_t::validate(self);

    if (!self->empty())
    {
        return &(self->lst[0]);
    }

    return nullptr;
}

breadcrumb_t *breadcrumb_list_t::newest(breadcrumb_list_t *self)
{
    if (!self || !self->on)
    {
        return nullptr;
    }

    breadcrumb_t *pointer = nullptr;
    size_t i;

    // Get the first valid breadcrumb.
    for (i = 0; i < self->count; ++i)
    {
        breadcrumb_t *bc = &(self->lst[i]);
        if (bc->valid)
        {
            pointer = bc;
            break;
        }
    }

    // Not a single valid breadcrumb was found.
    if (!pointer)
    {
        return nullptr;
    }

    // A valid breadcrumb was found. Check if there are newer valid breadcrumbs.
    for (i++; i < self->count; ++i)
    {
        breadcrumb_t *bc = &(self->lst[i]);
        if (bc->valid)
        {
            if (breadcrumb_t::isYounger(*bc, *pointer))
            {
                pointer = bc;
            }
        }
    }

    return pointer;
}

breadcrumb_t *breadcrumb_list_t::oldest(breadcrumb_list_t *self)
{
    if (!self || !self->on)
    {
        return nullptr;
    }

    breadcrumb_t *pointer = nullptr;
    size_t i;

    // Get the first valid breadcrumb.
    for (i = 0; i < self->count; ++i)
    {
        breadcrumb_t *bc = &(self->lst[i]);
        if (bc->valid)
        {
            pointer = bc;
            break;
        }
    }

    // Not a single valid breadcrumb was found.
    if (!pointer)
    {
        return nullptr;
    }

    // A valid breadcrumb was found. Check if there are older valid breadcrumbs.
    for (i++; i < self->count; ++i)
    {
        breadcrumb_t *bc = &(self->lst[i]);
        if (bc->valid)
        {
            if (breadcrumb_t::isOlder(*bc, *pointer))
            {
                pointer = bc;
            }
        }
    }


    return pointer;
}

breadcrumb_t *breadcrumb_list_t::oldest_grid(breadcrumb_list_t *self, Uint32 grid)
{
    if (!self || !self->on)
    {
        return nullptr;
    }

    breadcrumb_t *pointer = nullptr;
    size_t i;

    // Get the first valid breadcrumb for the given grid.
    for (i = 0; i < self->count; ++i)
    {
        breadcrumb_t *bc = &(self->lst[i]);

        if (bc->valid)
        {
            if (bc->grid == grid)
            {
                pointer = bc;
                break;
            }
        }
    }

    // Not a single valid breadcrumb for this grid was found.
    if (!pointer)
    {
        return nullptr;
    }

    // A valid breadcrumb for this grid was found.
    // Check if there are newer, valid breadcrumbs for this grid.
    for (i++; i < self->count; ++i)
    {
        breadcrumb_t *bc = &(self->lst[i]);
        if (bc->valid)
        {
            if (bc->grid == grid)
            {
                if (breadcrumb_t::isYounger(*bc, *pointer))
                {
                    pointer = bc;
                }
            }
        }
    }

    return pointer;
}

breadcrumb_t *breadcrumb_list_t::alloc(breadcrumb_list_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    // If the list is full ...
    if (self->full())
    {
        // ... try to compact it.
        self->compact();
    }

    // If the list is still full after compaction ...
    if (self->full())
    {
        // .. re-use the oldest element.
        return breadcrumb_list_t::oldest(self);
    }
    else
    {
        breadcrumb_t *breadcrumb = &(self->lst[self->count]);
        self->count++;
        breadcrumb->id = breadcrumb_guid++;
        return breadcrumb;
    }
}

bool breadcrumb_list_t::add(breadcrumb_list_t *self, breadcrumb_t *element)
{
    if (!self || !self->on || !element)
    {
        return false;
    }

    // If the list is full ...
    if (self->full())
    {
        // ... compact it.
        self->compact();
    }

    breadcrumb_t *old;
    // If the list is full after compaction ...
    if (self->full())
    {
        // we must reuse a breadcrumb:

        // Try the oldest breadcrumb for the given grid index.
        old = breadcrumb_list_t::oldest_grid(self, element->grid);

        if (!old)
        {
            // No element for the given grid index exists:
            // Try the oldest breadcrumb.
            old = breadcrumb_list_t::oldest(self);
        }
    }
    else
    {
        // The list is not full, so just allocate an element as normal.
        old = breadcrumb_list_t::alloc(self);
    }

    // If a breadcrumb is available ...
    if (old)
    {
        // ... assign the data.
        *old = *element;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
phys_data_t *phys_data_clear(phys_data_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    apos_t::reset(&(self->aplat));
    apos_t::reset(&(self->acoll));
    self->avel = fvec3_t::zero();
    /// @todo Seems like dynamic and loaded data are mixed here;
    /// We may not blank bumpdampen, weight or dampen for now.
#if 0
    self->bumpdampen = 1.0f;
    self->weight = 1.0f;
    self->dampen = 0.5f;
#endif
    return self;
}

void phys_data_t::reset(phys_data_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    apos_t::reset(&(self->aplat));
    apos_t::reset(&(self->acoll));
    self->avel = fvec3_t::zero();
    self->bumpdampen = 1.0f;
    self->weight = 1.0f;
    self->dampen = 0.5f;
}

phys_data_t *phys_data_t::ctor(phys_data_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    apos_t::ctor(&(self->aplat));
    apos_t::ctor(&(self->acoll));
    self->avel = fvec3_t::zero();
    self->bumpdampen = 1.0f;
    self->weight = 1.0f;
    self->dampen = 0.5f;

    return self;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool apos_t::self_union(apos_t *self, const apos_t *other)
{
    if (!self || !other)
    {
        return false;
    }
    // Scan through the components of the vector and find the maximum displacement.
    for (size_t i = 0; i < 3; ++i)
    {
        self->mins[i] = std::min(self->mins[i], other->mins[i]);
        self->maxs[i] = std::max(self->maxs[i], other->maxs[i]);
        self->sum[i] += other->sum[i];
    }

    return true;
}

bool apos_t::self_union(apos_t *self, const fvec3_t& other)
{
    if (!self)
    {
        return false;
    }
    // Scan through the components of the vector and find the maximum displacement.
    for (size_t i = 0; i < 3; ++i)
    {
        // Find the extrema of the displacement.
        if (other[i] > 0.0f)
        {
            self->maxs[i] = std::max(self->maxs[i], other[i]);
        }
        else if (other[i] < 0.0f)
        {
            self->mins[i] = std::min(self->mins[i], other[i]);
        }

        // Find the sum of the displacement.
        self->sum.v[i] += other[i];
    }

    return true;
}

bool apos_t::self_union_index(apos_t *self, const float t, const size_t index)
{
    // find the maximum displacement at the given index

    if (!self || index > 2)
    {
        return false;
    }

    LOG_NAN(t);

    // Update extrema.
    if (t > 0.0f )
    {
        self->maxs[index] = std::max(self->maxs[index], t);
    }
    else if (t < 0.0f)
    {
        self->mins[index] = std::min(self->mins[index], t);
    }

    // Update sum.
    self->sum[index] += t;

    return true;
}

bool apos_t::evaluate(const apos_t *self, fvec3_t& dst)
{
    if (!self)
    {
		dst = fvec3_t::zero();
		return true;
    }

    for (size_t i = 0; i < 3; ++i)
    {
        dst[i] = self->maxs[i] + self->mins[i];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
phys_data_t *phys_data_sum_aplat(phys_data_t *self, const fvec3_t& v)
{
    if (!self)
    {
        return nullptr;
    }
    apos_t::self_union(&(self->aplat ), v);
    return self;
}

phys_data_t *phys_data_sum_acoll(phys_data_t *self, const fvec3_t& v)
{
    if (!self)
    {
        return nullptr;
    }
    apos_t::self_union(&(self->acoll), v);
    return self;
}

phys_data_t *phys_data_sum_avel(phys_data_t *self, const fvec3_t& v)
{
    if (!self)
    {
        return nullptr;
    }
    self->avel += v;

    return self;
}

phys_data_t *phys_data_sum_aplat_index(phys_data_t *self, const float val, const size_t index)
{
    if (!self)
    {
        return nullptr;
    }

    LOG_NAN(val);

    apos_t::self_union_index(&(self->aplat), val, index);

    return self;
}

phys_data_t *phys_data_sum_acoll_index(phys_data_t *self, const float val, const size_t index)
{
    if (!self)
    {
        return nullptr;
    }

    LOG_NAN(val);

    apos_t::self_union_index(&(self->acoll), val, index);

    return self;
}

phys_data_t *phys_data_sum_avel_index(phys_data_t *self, const float val, const size_t index)
{
    if (!self || index > 2)
    {
        return nullptr;
    }

    LOG_NAN(val);

    self->avel.v[index] += val;

    return self;
}

//--------------------------------------------------------------------------------------------
//Inline below
//--------------------------------------------------------------------------------------------
bool test_interaction_close_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform )
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    cv_a.assign(bump_a);
    cv_b.assign(bump_b);

    return test_interaction_close_2(cv_a, pos_a, cv_b, pos_b, test_platform);
}

//--------------------------------------------------------------------------------------------
bool test_interaction_0(bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform)
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    cv_a.assign(bump_a);
    cv_b.assign(bump_b);

    return test_interaction_2(cv_a, pos_a, cv_b, pos_b, test_platform);
}

//--------------------------------------------------------------------------------------------
bool test_interaction_close_1(const oct_bb_t& cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform)
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    cv_b.assign(bump_b);

    return test_interaction_close_2(cv_a, pos_a, cv_b, pos_b, test_platform);
}

//--------------------------------------------------------------------------------------------
bool test_interaction_1(const oct_bb_t& cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, int test_platform)
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    cv_b.assign(bump_b);

    return test_interaction_2(cv_a, pos_a, cv_b, pos_b, test_platform);
}

//--------------------------------------------------------------------------------------------
bool test_interaction_close_2(const oct_bb_t& cv_a, const fvec3_t& pos_a, const oct_bb_t& cv_b, const fvec3_t& pos_b, int test_platform)
{
#if 0
    if (!cv_a || !cv_b) return false;
#endif
    // Translate the vector positions to octagonal vector positions.
    oct_vec_v2_t oa(pos_a), ob(pos_b);

    // calculate the depth
    float depth;
    for (size_t i = 0; i < OCT_Z; ++i)
    {
        float ftmp1 = std::min((ob[i] + cv_b.maxs[i]) - oa[i], oa[i] - (ob[i] + cv_b.mins[i]));
        float ftmp2 = std::min((oa[i] + cv_a.maxs[i]) - ob[i], ob[i] - (oa[i] + cv_a.mins[i]));
        depth = std::max(ftmp1, ftmp2);
        if (depth <= 0.0f) return false;
    }

    // treat the z coordinate the same as always
    depth = std::min(cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z]) -
            std::max(cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z]);

    return TO_C_BOOL(test_platform ? (depth > -PLATTOLERANCE) : (depth > 0.0f));
}

//--------------------------------------------------------------------------------------------
bool test_interaction_2(const oct_bb_t& cv_a, const fvec3_t& pos_a, const oct_bb_t& cv_b, const fvec3_t& pos_b, int test_platform)
{
#if 0
    if (!cv_a || !cv_b)
    {
        return false;
    }
#endif

    // Convert the vector positions to octagonal vector positions.
    oct_vec_v2_t oa(pos_a), ob(pos_b);

    // calculate the depth
    float depth;
    for (size_t i = 0; i < OCT_Z; ++i)
    {
        depth  = std::min(cv_b.maxs[i] + ob[i], cv_a.maxs[i] + oa[i]) -
                 std::max(cv_b.mins[i] + ob[i], cv_a.mins[i] + oa[i]);

        if (depth <= 0.0f) return false;
    }

    // treat the z coordinate the same as always
    depth = std::min(cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z]) -
            std::max(cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z]);

    return TO_C_BOOL((0 != test_platform) ? (depth > -PLATTOLERANCE) : (depth > 0.0f));
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if 0
bool get_depth_close_0( bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth )
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    cv_a.assign(bump_a);
    cv_b.assign(bump_b);

    // shift the bumpers
    cv_a.translate( pos_a );
    cv_b.translate( pos_b );

    return get_depth_close_2( &cv_a, &cv_b, break_out, depth );
}
#endif
//--------------------------------------------------------------------------------------------
bool get_depth_0(bumper_t bump_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth)
{
    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    cv_a.assign(bump_a);
    cv_b.assign(bump_b);

    return get_depth_2(cv_a, pos_a, cv_b, pos_b, break_out, depth);
}

//--------------------------------------------------------------------------------------------
#if 0
bool get_depth_close_1(const oct_bb_t& cv_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth)
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    cv_b.assign(bump_b);

    // shift the bumper
    cv_b.translate(pos_b);

    return get_depth_close_2(cv_a, cv_b, break_out, depth);
}
#endif
//--------------------------------------------------------------------------------------------
bool get_depth_1(const oct_bb_t& cv_a, const fvec3_t& pos_a, bumper_t bump_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth)
{
    oct_bb_t cv_b;

    // convert the bumper to the correct format
    cv_b.assign(bump_b);

    return get_depth_2(cv_a, pos_a, cv_b, pos_b, break_out, depth);
}

//--------------------------------------------------------------------------------------------
#if 0
bool get_depth_close_2(const oct_bb_t& cv_a, const oct_bb_t& cv_b, bool break_out, oct_vec_v2_t& depth)
{
    // calculate the depth
    bool valid = true;
    for (size_t i = 0; i < OCT_Z; ++i)
    {
        // get positions from the bounding volumes
        float opos_a = (cv_a.mins[i] + cv_a->maxs[i]) * 0.5f;
        float opos_b = (cv_b.mins[i] + cv_b->maxs[i]) * 0.5f;

        // measue the depth
        float ftmp1 = std::min(cv_b.maxs[i] - opos_a, opos_a - cv_b.mins[i]);
        float ftmp2 = std::min(cv_a.maxs[i] - opos_b, opos_b - cv_a.mins[i]);
        depth[i] = std::max( ftmp1, ftmp2 );

        if ( depth[i] <= 0.0f )
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
    depth[OCT_XY] *= Ego::Math::invSqrtTwo<float>();
    depth[OCT_YX] *= Ego::Math::invSqrtTwo<float>();

    return valid;
}
#endif
//--------------------------------------------------------------------------------------------
bool get_depth_2(const oct_bb_t& cv_a, const fvec3_t& pos_a, const oct_bb_t& cv_b, const fvec3_t& pos_b, bool break_out, oct_vec_v2_t& depth)
{
#if 0
    if (!cv_a || !cv_b) return false;
#endif
    // Translate the vector positions to octagonal vector positions.
    oct_vec_v2_t oa(pos_a), ob(pos_b);

    // calculate the depth
    bool valid = true;
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        depth[i] = std::min(cv_b.maxs[i] + ob[i], cv_a.maxs[i] + oa[i]) -
                   std::max(cv_b.mins[i] + ob[i], cv_a.mins[i] + oa[i]);

        if (depth[i] <= 0.0f)
        {
            valid = false;
            if (break_out) return false;
        }
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= Ego::Math::invSqrtTwo<float>();
    depth[OCT_YX] *= Ego::Math::invSqrtTwo<float>();

    return valid;
}
