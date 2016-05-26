#pragma once

#include "egolib/Math/Intersects_AxisAlignedBox_Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a point and an axis aligned box intersect.
 * @remark The Intersects<AxisAlignedBox<EuclideanSpaceType>,Point<typename EuclideanSpaceType::VectorSpaceType>> functor is re-used.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    Point<typename EuclideanSpaceType::VectorSpaceType>,
    AxisAlignedBox<EuclideanSpaceType>
> {
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> FirstType;
    typedef AxisAlignedBox<EuclideanSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        static const Intersects<SecondType, FirstType> functor;
        return functor(b, a);
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
