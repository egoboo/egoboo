#pragma once

#include "egolib/Math/Intersects.hpp"
#include "egolib/Math/Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if two points intersect.
 */
template<typename EuclideanSpaceType> 
struct Intersects<
    Point<EuclideanSpaceType>,
    Point<EuclideanSpaceType>
> {
    typedef Point<EuclideanSpaceType> Type;
    bool operator()(const Type& a, const Type& b) const {
        for (size_t i = 0, n = EuclideanSpaceType::dimensionality(); i < n; ++i) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
