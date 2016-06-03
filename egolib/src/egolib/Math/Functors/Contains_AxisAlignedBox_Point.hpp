#pragma once

#include "egolib/Math/Functors/Contains.hpp"
#include "egolib/Math/AxisAlignedBox.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if an axis aligned box contains a point.
 */
template <typename EuclideanSpaceType>
struct Contains<
    AxisAlignedBox<EuclideanSpaceType>,
    Point<typename EuclideanSpaceType::VectorSpaceType>
> {
    typedef AxisAlignedBox<EuclideanSpaceType> FirstType;
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality(); ++i) {
            if (a.getMax()[i] < b[i]) return false;
            if (a.getMin()[i] > b[i]) return false;
        }
        return true;
    }
}; // struct Contains

} // namespace Math
} // namespace Ego
