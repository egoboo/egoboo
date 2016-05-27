#pragma once

#include "egolib/Math/Intersects.hpp"
#include "egolib/Math/AxisAlignedCube.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if an axis aligned cube and a point intersect.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    AxisAlignedCube<EuclideanSpaceType>,
    Point<typename EuclideanSpaceType::VectorSpaceType>
> {
    typedef AxisAlignedCube<EuclideanSpaceType> FirstType;
    typedef Point<typename EuclideanSpaceType::VectorSpaceType> SecondType;
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
