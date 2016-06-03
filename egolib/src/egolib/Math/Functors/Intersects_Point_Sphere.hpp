#pragma once

#include "egolib/Math/Functors/Intersects_Sphere_Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines wether a sphere and a point intersect.
 * @remark The Intersects<Sphere<EuclideanSpaceType>,Point<typename EuclideanSpaceType::VectorSpaceType>> functor is re-used.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    Point<typename EuclideanSpaceType::VectorSpaceType>,
    Sphere<EuclideanSpaceType>
> {
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> FirstType;
    typedef Sphere<EuclideanSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        static const Intersects<SecondType, FirstType> functor;
        return functor(b, a);
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
