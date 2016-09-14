#pragma once

#include "egolib/Math/Functors/Intersects_AxisAlignedBox_AxisAlignedCube.hpp"

namespace Ego {
namespace Math {

/** 
 * @brief Functor which determines if an axis aligned cube and an axis aligned box intersect.
 * @remark The
 * Intersects<AxisAlignedBox<EuclideanSpaceType>,AxisAlignedCube<EuclideanSpaceType>>
 * functor is re-used.
 */
template <typename EuclideanSpaceType>
struct Intersects<
	AxisAlignedCube<EuclideanSpaceType>,
	AxisAlignedBox<EuclideanSpaceType>
> {
    using FirstType = AxisAlignedCube<EuclideanSpaceType>;
	using SecondType = AxisAlignedBox<EuclideanSpaceType>;
    bool operator()(const FirstType& a, const SecondType& b) const {
        static const Intersects<SecondType,FirstType> functor;
		return functor(b,a);
    }
}; // Intersects

} // namespace Math
} // namespace Ego
