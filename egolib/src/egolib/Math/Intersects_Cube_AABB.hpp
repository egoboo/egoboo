#pragma once

#include "egolib/Math/Intersects_AABB_Cube.hpp"

namespace Ego {
namespace Math {

/** 
 * @brief Functor which determines if a cube and an axis-aligned bounding box intersect.
 * @remark The Intersects<AABB<EuclideanSpaceType>,Cube<EuclideanSpaceType>> functor is re-used.
 */
template <typename EuclideanSpaceType>
struct Intersects<
	Cube<EuclideanSpaceType>,
	AABB<EuclideanSpaceType>
> {
    typedef Cube<EuclideanSpaceType> FirstType;
	typedef AABB<EuclideanSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        static const Intersects<SecondType,FirstType> functor;
		return functor(b,a);
    }
}; // Intersects

} // namespace Math
} // namespace Ego
