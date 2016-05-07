#pragma once

#include "egolib/Math/Intersects_Cube_Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a point and a cube intersect.
 * @remark The Intersects<Cube<EuclideanSpaceType>,Vector<typename EuclideanSpaceType::VectorSpaceType>> functor is re-used.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    Point<typename EuclideanSpaceType::VectorSpaceType>,
    Cube<EuclideanSpaceType>
> {
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> FirstType;
    typedef Cube<EuclideanSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        static const Intersects<SecondType, FirstType> functor;
        return functor(b, a);
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
