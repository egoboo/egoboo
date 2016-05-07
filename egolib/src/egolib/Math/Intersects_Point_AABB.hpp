#pragma once

#include "egolib/Math/Intersects_AABB_Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a point and an axis-aligned bounding box intersect.
 * @remark The Intersects<AABB<EuclideanSpaceType>,Vector<typename EuclideanSpaceType::VectorSpaceType>> functor is re-used.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    Point<typename EuclideanSpaceType::VectorSpaceType>,
    AABB<EuclideanSpaceType>
> {
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> FirstType;
    typedef AABB<EuclideanSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        static const Intersects<SecondType, FirstType> functor;
        return functor(b, a);
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
