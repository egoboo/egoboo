#pragma once

#include "egolib/Math/Functors/Intersects.hpp"
#include "egolib/Math/Sphere.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines wether two spheres intersect.
 * @remark Two spheres \f$(c_0,r_0)\f$ and \f$(c_1,r_1)\f$ with the
 * centers \f$c_0\f$ and \f$c_1\f$ and the radii \f$r_0\f$
 * and \f$r_1\f$ intersect if \f$|c_1 - c_0| \leq r_0 + r_1\f$
 * holds. That condition is equivalent to the condition
 * \f$|c_1 - c_0|^2 \leq (r_0 + r_1)^2\f$ but the latter
 * is more efficient to test (two multiplications vs. one
 * square root).
 */
template <typename EuclideanSpaceType>
struct Intersects<
    Sphere<EuclideanSpaceType>,
    Sphere<EuclideanSpaceType>
> {
    typedef Sphere<EuclideanSpaceType> Type;
    bool operator()(const Type& a, const Type& b) const {
        // Get the squared distance between the centers of the two spheres.
        float distanceSquared = (a.getCenter() - b.getCenter()).length_2();
        // Get the squared sum of the radiis of the two spheres.
        float sumOfRadii = a.getRadius() + b.getRadius();
        float sumOfRadiiSquared = sumOfRadii * sumOfRadii;
        // If the squared distance beween the centers of the spheres
        // is smaller than or equal to the squared sum of the radii of the spheres ...
        if (distanceSquared <= sumOfRadiiSquared) {
            // ... the spheres intersect.
            return true;
        }
        // Otherwise they don't intersect.
        return false;
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
