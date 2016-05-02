#pragma once

#include "egolib/Math/Contains.hpp"
#include "egolib/Math/Sphere.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a sphere contains another sphere.
 * A sphere \f$S\f$ with center \f$c\f$ and radius \f$r\f$ contains
 * another sphere \f$S'\$ with center \f$c'\f$ and radius \f$r'\f$
 * if \f$|c-c'|+r' \leq r\f$ holds.
 */
template <typename EuclideanSpaceType>
struct Contains<
    Sphere<EuclideanSpaceType>,
    Sphere<EuclideanSpaceType>
> {
    typedef Sphere<EuclideanSpaceType> Type;
    bool operator()(const Type& a, const Type& b) const {
        return (a.getCenter() - b.getCenter()).length() + b.getRadius() <= a.getRadius();
    }
}; // struct Contains

} // namespace Math
} // namespace Ego
