#pragma once

#include "egolib/Math/Contains.hpp"
#include "egolib/Math/Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a point contains another point.
 */
template <typename VectorSpaceType>
struct Contains<
    Point<VectorSpaceType>,
    Point<VectorSpaceType>
> {
    typedef Point<VectorSpaceType> Type;
    bool operator()(const Type& a, const Type& b) const {
        for (size_t i = 0, n = VectorSpaceType::dimensionality(); i < n; ++i) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }
}; // struct Contains

} // namespace Math
} // namespace Ego
