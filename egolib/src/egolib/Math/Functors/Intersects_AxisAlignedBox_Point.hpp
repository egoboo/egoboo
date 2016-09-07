#pragma once

#include "egolib/Math/Functors/Intersects.hpp"
#include "egolib/Math/AxisAlignedBox.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if an axis aligned box and a point intersect.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    AxisAlignedBox<EuclideanSpaceType>,
    Point<typename EuclideanSpaceType::VectorSpaceType>
> {
    using FirstType = AxisAlignedBox<EuclideanSpaceType>;
    using SecondType = Point<typename EuclideanSpaceType::VectorSpaceType>;
    bool operator()(const FirstType& a, const SecondType& b) const {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            if (a.getMax()[i] < b[i]) return false;
            if (a.getMin()[i] > b[i]) return false;
        }
        return true;
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
