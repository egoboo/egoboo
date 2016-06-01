#pragma once

#include "egolib/Math/Functors/Intersects_AxisAlignedCube_Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a point and an axis aligned cube intersect.
 * @remark The Intersects<Cube<EuclideanSpaceType>,Point<typename EuclideanSpaceType::VectorSpaceType>> functor is re-used.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    Point<typename EuclideanSpaceType::VectorSpaceType>,
    AxisAlignedCube<EuclideanSpaceType>
> {
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> FirstType;
    typedef AxisAlignedCube<EuclideanSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        static const Intersects<SecondType, FirstType> functor;
        return functor(b, a);
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
