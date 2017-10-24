#pragma once

#include "egolib/platform.h"

namespace id {

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if an axis aligned bounding box intersects an axis aligned cube.
/// @remark An axis-aligned bounding box \f$X\f$ does not intersect a cube \f$Y\f$
/// if for at least one axis \f$k\f$ one of the following conditions is true:
/// - \f$X_{min_k} > Y_{max_k}\f$
/// - \f$X_{max_k} < Y_{min_k}\f$
/// Otherwise \f$X\f$ and \f$Y\f$ intersect.
/// This is a variant of the Separating Axis Theorem (aka SAT).
/// @tparam P the point type of the geometry types
template <typename P>
struct is_intersecting_functor<axis_aligned_box<P>, axis_aligned_cube<P>>
{
    bool operator()(const axis_aligned_box<P>& a, const axis_aligned_cube<P>& b) const
	{
        for (size_t i = 0; i < P::dimensionality();  ++i)
		{
            // If the minimum of a is greater than the maximum of b along one axis,
            // then they can not intersect.
            if (a.get_min()[i] > b.get_max()[i])
			{
                return false;
            }
            // If the maximum of a is smaller than the minimum of b along one axis,
            // then they can not intersect.
            if (a.get_max()[i] < b.get_min()[i])
			{
                return false;
            }
        }
        return true;
    }
}; // struct is_intersecting_functor

/// @brief Specialization of id::is_intersecting_functor
/// Determines if an axis aligned cube and an axis aligned box intersect.
/// @remark The method which determines wether an axis aligned box and
/// an axis aligned cube intersect is re-used.
/// @tparam P the point type of the geometry types
template <typename P>
struct is_intersecting_functor<axis_aligned_cube<P>, axis_aligned_box<P>>
{
	bool operator()(const axis_aligned_cube<P>& a, const axis_aligned_box<P>& b) const
	{ return is_intersecting(b, a); }
}; // is_intersecting_functor

} // namespace id
