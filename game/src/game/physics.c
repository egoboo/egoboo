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
#include "game/mesh.h"
#include "game/Entities/_Include.hpp"
#include "egolib/Float.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Ego::Physics::Environment Ego::Physics::g_environment;

static egolib_rv phys_intersect_oct_bb_index(int index, const oct_bb_t& src1, const oct_vec_v2_t& ovel1, const oct_bb_t& src2, const oct_vec_v2_t& ovel2, int test_platform, float *tmin, float *tmax);
static egolib_rv phys_intersect_oct_bb_close_index(int index, const oct_bb_t& src1, const oct_vec_v2_t& ovel1, const oct_bb_t& src2, const oct_vec_v2_t& ovel2, int test_platform, float *tmin, float *tmax);

/// @brief A test to determine whether two "fast moving" objects are interacting within a frame.
///        Designed to determine whether a bullet particle will interact with character.
//static bool phys_intersect_oct_bb_close(const oct_bb_t& src1_orig, const Vector3f& pos1, const Vector3f& vel1, const oct_bb_t& src2_orig, const Vector3f& pos2, const Vector3f& vel2, int test_platform, oct_bb_t& dst, float *tmin, float *tmax);
static bool phys_estimate_depth(const oct_vec_v2_t& odepth, const float exponent, Vector3f& nrm, float& depth);
//static float phys_get_depth(const oct_vec_v2_t& odepth, const Vector3f& nrm);
static bool phys_warp_normal(const float exponent, Vector3f& nrm);
static bool phys_get_pressure_depth(const oct_bb_t& bb_a, const oct_bb_t& bb_b, oct_vec_v2_t& odepth);
static bool phys_get_collision_depth(const oct_bb_t& bb_a, const oct_bb_t& bb_b, oct_vec_v2_t& odepth);

const Facing orientation_t::MAP_TURN_OFFSET(0x8000);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool phys_get_collision_depth(const oct_bb_t& bb_a, const oct_bb_t& bb_b, oct_vec_v2_t& odepth)
{
    odepth = oct_vec_v2_t();

    // are the initial volumes any good?
    if (bb_a._empty || bb_b._empty) return false;

    // is there any overlap?
    oct_bb_t otmp;
    if (rv_success != oct_bb_t::intersection(bb_a, bb_b, otmp))
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
        float fdepth = otmp._maxs[i] - otmp._mins[i];

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
    odepth = oct_vec_v2_t();

    // assume the best
    bool result = true;

    // scan through the dimensions of the oct_bbs
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        float diff1 = bb_a._maxs[i] - bb_b._mins[i];
        float diff2 = bb_b._maxs[i] - bb_a._mins[i];

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
bool phys_warp_normal(const float exponent, Vector3f& nrm)
{
    // use the exponent to warp the normal into a cylinder-like shape, if needed

    if (1.0f == exponent) return true;

    if (0.0f == nrm.length_abs()) return false;

    float length_hrz_2 = Vector2f(nrm[kX],nrm[kY]).length_2();
    float length_vrt_2 = nrm.length_2() - length_hrz_2;

    nrm[kX] = nrm[kX] * std::pow( length_hrz_2, 0.5f * ( exponent - 1.0f ) );
    nrm[kY] = nrm[kY] * std::pow( length_hrz_2, 0.5f * ( exponent - 1.0f ) );
    nrm[kZ] = nrm[kZ] * std::pow( length_vrt_2, 0.5f * ( exponent - 1.0f ) );

    // normalize the normal
	nrm.normalize();
    return nrm.length() >= 0.0f;
}

//--------------------------------------------------------------------------------------------
bool phys_estimate_depth(const oct_vec_v2_t& odepth, const float exponent, Vector3f& nrm, float& depth)
{
    // use the given (signed) podepth info to make a normal vector, and measure
    // the shortest distance to the border

	Vector3f nrm_aa;

    if (0.0f != odepth[OCT_X]) nrm_aa[kX] = 1.0f / odepth[OCT_X];
    if (0.0f != odepth[OCT_Y]) nrm_aa[kY] = 1.0f / odepth[OCT_Y];
    if (0.0f != odepth[OCT_Z]) nrm_aa[kZ] = 1.0f / odepth[OCT_Z];

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

    if (nrm_aa[kX] != 0.0f)
    {
        float ftmp = odepth[OCT_X] / nrm_aa[kX];
        ftmp = std::max(0.0f, ftmp);
        tmin_aa = std::min(tmin_aa, ftmp);
    }

    if (nrm_aa[kY] != 0.0f)
    {
        float ftmp = odepth[OCT_Y] / nrm_aa[kY];
        ftmp = std::max(0.0f, ftmp);
        tmin_aa = std::min(tmin_aa, ftmp);
    }

    if (nrm_aa[kZ] != 0.0f)
    {
        float ftmp = odepth[OCT_Z] / nrm_aa[kZ];
        ftmp = std::max(0.0f, ftmp);
        tmin_aa = std::min(tmin_aa, ftmp);
    }

    if (tmin_aa <= 0.0f || tmin_aa >= 1e6) return false;

    // Next do the diagonal axes.
	Vector3f nrm_diag = Vector3f::zero();

    if (0.0f != odepth[OCT_XY]) nrm_diag[kX] = 1.0f / (odepth[OCT_XY] * Ego::Math::invSqrtTwo<float>());
    if (0.0f != odepth[OCT_YX]) nrm_diag[kY] = 1.0f / (odepth[OCT_YX] * Ego::Math::invSqrtTwo<float>());
    if (0.0f != odepth[OCT_Z ]) nrm_diag[kZ] = 1.0f / odepth[OCT_Z];

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

    if (nrm_diag[kX] != 0.0f)
    {
        float ftmp = Ego::Math::invSqrtTwo<float>() * odepth[OCT_XY] / nrm_diag[kX];
        ftmp = std::max(0.0f, ftmp);
        tmin_diag = std::min(tmin_diag, ftmp);
    }

    if (nrm_diag[kY] != 0.0f)
    {
        float ftmp = Ego::Math::invSqrtTwo<float>() * odepth[OCT_YX] / nrm_diag[kY];
        ftmp = std::max(0.0f, ftmp);
        tmin_diag = std::min(tmin_diag, ftmp);
    }

    if (nrm_diag[kZ] != 0.0f)
    {
        float ftmp = odepth[OCT_Z] / nrm_diag[kZ];
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
        nrm[kX] = (nrm_diag[kX] - nrm_diag[kY]) * Ego::Math::invSqrtTwo<float>();
        nrm[kY] = (nrm_diag[kX] + nrm_diag[kY]) * Ego::Math::invSqrtTwo<float>();
        nrm[kZ] = nrm_diag[kZ];
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
bool phys_estimate_collision_normal(const oct_bb_t& obb_a, const oct_bb_t& obb_b, const float exponent, oct_vec_v2_t& odepth, Vector3f& nrm, float& depth)
{
    // estimate the normal for collision volumes that are partially overlapping

    // Do we need to use the more expensive algorithm?
    bool use_pressure = false;
    if (oct_bb_t::contains(obb_a, obb_b))
    {
        use_pressure = true;
    }
    else if (oct_bb_t::contains(obb_b, obb_a))
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
bool phys_estimate_pressure_normal(const oct_bb_t& obb_a, const oct_bb_t& obb_b, const float exponent, oct_vec_v2_t& odepth, Vector3f& nrm, float& depth)
{
    // use a more robust algorithm to get the normal no matter how the 2 volumes are
    // related

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

    float vdiff = ovel2[index] - ovel1[index];
    if ( 0.0f == vdiff ) return rv_fail;

    float src1_min = src1._mins[index];
    float src1_max = src1._maxs[index];
    float src2_min = src2._mins[index];
    float src2_max = src2._maxs[index];

    if (OCT_Z != index)
    {
        // Is there any possibility of the 2 objects acting as a platform pair.
        bool close_test_1 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ1);
        bool close_test_2 = HAS_SOME_BITS(test_platform, PHYS_PLATFORM_OBJ2);

        // Only do a close test if the object's feet are above the platform.
        close_test_1 = close_test_1 && (src1._mins[OCT_Z] > src2._maxs[OCT_Z]);
        close_test_2 = close_test_2 && (src2._mins[OCT_Z] > src1._maxs[OCT_Z]);

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
bool phys_intersect_oct_bb(const oct_bb_t& src1_orig, const Vector3f& pos1, const Vector3f& vel1, const oct_bb_t& src2_orig, const Vector3f& pos2, const Vector3f& vel2, int test_platform, oct_bb_t& dst, float *tmin, float *tmax)
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
    oct_bb_t::translate(src1_orig, opos1, src1);
    oct_bb_t::translate(src2_orig, opos2, src2);

    bool found = false;
    *tmin = +1.0e6;
    *tmax = -1.0e6;

    int failure_count = 0;
    if ((vel1-vel2).length_abs() < 1.0e-6)
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
                    failure_count++;
                }
                else if (rv_success == retval)
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
        oct_bb_t::intersection(src1, src2, dst);
    }
    else
    {
        float tmp_min, tmp_max;

        // Check to see if there the intersection times make any sense.
        if (*tmax <= *tmin) return false;

        // Check whether there is any overlap this frame.
        if (*tmin >= 1.0f || *tmax <= 0.0f) return false;

        // Clip the interaction time to just one frame.
        tmp_min = Ego::Math::constrain(*tmin, 0.0f, 1.0f);
        tmp_max = Ego::Math::constrain(*tmax, 0.0f, 1.0f);

        // determine the expanded collision volumes for both objects (for this frame)
        oct_bb_t exp1, exp2;
        phys_expand_oct_bb(src1, vel1, tmp_min, tmp_max, exp1);
        phys_expand_oct_bb(src2, vel2, tmp_min, tmp_max, exp2);

        // determine the intersection of these two expanded volumes (for this frame)
        oct_bb_t::intersection(exp1, exp2, dst);
    }

    if (0 != test_platform)
    {
        dst._maxs[OCT_Z] += PLATTOLERANCE;
        dst._empty = oct_bb_t::empty_raw(dst);
    }

    if (dst._empty) return false;

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
    float src1_min = src1._mins[index];
    float src1_max = src1._maxs[index];
    float opos1 = (src1_min + src1_max) * 0.5f;

    /// @todo Use src2.getMin(index), src2.getMax(index) and src2.getMid(index).
    float src2_min = src2._mins[index];
    float src2_max = src2._maxs[index];
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
bool phys_expand_oct_bb(const oct_bb_t& src, const Vector3f& vel, const float tmin, const float tmax, oct_bb_t& dst)
{
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
		Vector3f tmp_diff = vel * tmin;
        // Adjust the bounding box to take in the position at the next step.
        oct_bb_t::translate(src, tmp_diff, tmp_min);
    }

    // Determine the bounding volume at t == tmax.
    if ( tmax == 0.0f )
    {
        tmp_max = src;
    }
    else
    {
		Vector3f tmp_diff = vel * tmax;
        // Adjust the bounding box to take in the position at the next step.
		oct_bb_t::translate(src, tmp_diff, tmp_max);
	}

    // Determine bounding box for the range of times.
	oct_bb_t::join(tmp_min, tmp_max, dst);

    return true;
}

//--------------------------------------------------------------------------------------------
bool phys_expand_chr_bb(Object *pchr, float tmin, float tmax, oct_bb_t& dst)
{
    if (!pchr || pchr->isTerminated()) return false;

    // copy the volume
    oct_bb_t tmp_oct1 = pchr->chr_max_cv;

    // add in the current position to the bounding volume
    oct_bb_t tmp_oct2;
    oct_bb_t::translate(tmp_oct1, pchr->getPosition(), tmp_oct2);

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb(tmp_oct2, pchr->vel, tmin, tmax, dst);
}

//--------------------------------------------------------------------------------------------
bool phys_expand_prt_bb(Ego::Particle *pprt, float tmin, float tmax, oct_bb_t& dst)
{
    /// @author BB
    /// @details use the object velocity to figure out where the volume that the particle will
    ///               occupy during this update
    if(!pprt || pprt->isTerminated()) return false;

    // copy the volume
    oct_bb_t tmp_oct1 = pprt->prt_max_cv;

    // add in the current position to the bounding volume
    oct_bb_t tmp_oct2;
    oct_bb_t::translate(tmp_oct1, pprt->getPosition(), tmp_oct2);

    // streach the bounging volume to cover the path of the object
    return phys_expand_oct_bb(tmp_oct2, pprt->vel, tmin, tmax, dst);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if 0
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
static Vector3f snap(const Vector3f& p)
{
    return Vector3f((std::floor(p[kX] / GRID_FSIZE) + 0.5f) * GRID_FSIZE,
                    (std::floor(p[kY] / GRID_FSIZE) + 0.5f) * GRID_FSIZE,
                    p[kZ]);
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void phys_data_t::clear()
{
	aplat = apos_t();
	acoll = apos_t();
    avel = Vector3f::zero();
    /// @todo Seems like dynamic and loaded data are mixed here;
    /// We may not blank bumpdampen, weight or dampen for now.
#if 0
    bumpdampen = 1.0f;
    weight = 1.0f;
    dampen = 0.5f;
#endif
}

phys_data_t::phys_data_t()
	: aplat(), acoll(), avel(),
	  bumpdampen(1.0f), weight(1.0f), dampen(0.5f)
{}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void apos_t::join(const apos_t& other) {
    for (size_t i = 0; i < 3; ++i) {
        mins[i] = std::min(mins[i], other.mins[i]);
        maxs[i] = std::max(maxs[i], other.maxs[i]);
        sum[i] += other.sum[i];
    }
}

void apos_t::join(const Vector3f& other) {
    for (size_t i = 0; i < 3; ++i) {
        if (other[i] > 0.0f) {
            maxs[i] = std::max(maxs[i], other[i]);
        } else if (other[i] < 0.0f) {
            mins[i] = std::min(mins[i], other[i]);
        }
        sum[i] += other[i];
    }
}

void apos_t::join(const float t, const size_t index) {
    if (index > 2) {
		throw std::runtime_error("index out of bounds");
    }

    LOG_NAN(t);

    if (t > 0.0f) {
        maxs[index] = std::max(maxs[index], t);
    } else if (t < 0.0f) {
        mins[index] = std::min(mins[index], t);
    }

	sum[index] += t;
}

void apos_t::evaluate(const apos_t& self, Vector3f& dst) {
    for (size_t i = 0; i < 3; ++i) {
        dst[i] = self.maxs[i] + self.mins[i];
    }
}

//--------------------------------------------------------------------------------------------

void phys_data_t::sum_acoll(const Vector3f& v)
{
    acoll.join(v);
}

void phys_data_t::sum_avel(const Vector3f& v)
{
    avel += v;
}

void phys_data_t::sum_aplat(const float val, const size_t index)
{
	if (index > 2)
	{
		throw std::runtime_error("index out of bounds");
	}

    LOG_NAN(val);

    aplat.join(val, index);
}

void phys_data_t::sum_avel(const float val, const size_t index)
{
    if (index > 2)
    {
		throw std::runtime_error("index out of bounds");
    }

    LOG_NAN(val);

    avel[index] += val;
}
