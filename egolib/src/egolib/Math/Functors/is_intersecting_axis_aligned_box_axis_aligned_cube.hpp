#pragma once

#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/AxisAlignedCube.hpp"

namespace id {

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if an axis aligned bounding box intersects an axis aligned cube.
/// @remark An axis-aligned bounding box \f$X\f$ does not intersect a cube \f$Y\f$
/// if for at least one axis \f$k\f$ one of the following conditions is true:
/// - \f$X_{min_k} > Y_{max_k}\f$
/// - \f$X_{max_k} < Y_{min_k}\f$
/// Otherwise \f$X\f$ and \f$Y\f$ intersect.
/// This is a variant of the Separating Axis Theorem (aka SAT).
template <typename E>
struct is_intersecting_functor<Ego::Math::AxisAlignedBox<E>,
                               Ego::Math::AxisAlignedCube<E>>
{
    bool operator()(const Ego::Math::AxisAlignedBox<E>& a, 
		            const Ego::Math::AxisAlignedCube<E>& b) const
	{
        for (size_t i = 0; i < E::dimensionality();  ++i) {
            // If the minimum of a is greater than the maximum of b along one axis,
            // then they can not intersect.
            if (a.getMin()[i] > b.getMax()[i])
			{
                return false;
            }
            // If the maximum of a is smaller than the minimum of b along one axis,
            // then they can not intersect.
            if (a.getMax()[i] < b.getMin()[i])
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
template <typename E>
struct is_intersecting_functor<Ego::Math::AxisAlignedCube<E>,
	                           Ego::Math::AxisAlignedBox<E>>
{
	bool operator()(const Ego::Math::AxisAlignedCube<E>& a,
		            const Ego::Math::AxisAlignedBox<E>& b) const
	{
		return is_intersecting(b, a);
	}
}; // is_intersecting_functor

} // namespace id
