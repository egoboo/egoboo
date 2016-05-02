#pragma once

#include "egolib/Math/Contains.hpp"
#include "egolib/Math/Sphere.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines wether a sphere contains a point.
 * @remark A sphere \f$(c,r)\f$ with the center $c$ and the radius $r$
 * contains a point \f$p\f$ if \f$|p - c| \leq r\f$ holds.
 * That condition is equivalent to the condition \f$|p - c|^2
 * \leq r^2\f$ but the latter is more efficient to test (two
 * multiplications vs. one square root).
 */
template <typename EuclideanSpaceType>
struct Contains<
    Sphere<EuclideanSpaceType>,
    Point<typename EuclideanSpaceType::VectorSpaceType>
> {
    typedef Sphere<EuclideanSpaceType> FirstType;
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        // Get the squared distance from the point to the center of the sphere.
        float distanceSquared = (a.getCenter() - b).length_2();
        // Get the squared radius of the sphere.
        float radiusSquared = a.getRadiusSquared();
        // If the squared distance beween the point and the center of the sphere
        // is smaller than or equal to the squared radius of the sphere ...
        if (distanceSquared <= radiusSquared) {
            // ... the sphere contains the point.
            return true;
        }
        // Otherwise the sphere does not contain the point.
        return false;
    }
}; // struct Contains

} // namespace Math
} // namespace Ego
