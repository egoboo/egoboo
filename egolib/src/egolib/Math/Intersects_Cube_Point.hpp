#pragma once

#include "egolib/Math/Intersects.hpp"
#include "egolib/Math/Cube.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a cube and a point intersect.
 */
template <typename EuclideanSpaceType>
struct Intersects<
    Cube<EuclideanSpaceType>,
    Point<typename EuclideanSpaceType::VectorSpaceType>
> {
    typedef Cube<EuclideanSpaceType> FirstType;
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
