#pragma once

#include "egolib/Math/Intersects.hpp"
#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/AxisAlignedCube.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Functor which determines if a axis-aligned bounding box intersects with a cube.
 * @remark An axis-aligned bounding box \f$a\f$ does not intersect with a cube \f$b\f$
 * if for at least one axis \f$k\f$ one of the following conditions is true:
 * - \f$x_{min_k} > y_{max_k}\f$
 * - \f$x_{max_k} < y_{min_k}\f$
 * Otherwise \f$x\f$ and \f$y\f$ intersect.
 * This is a variant of the Separating Axis Theorem (aka SAT).
 */
template <typename EuclideanSpaceType>
struct Intersects<
    AxisAlignedBox<EuclideanSpaceType>,
    AxisAlignedCube<EuclideanSpaceType>
> {
    typedef AxisAlignedBox<EuclideanSpaceType> FirstType;
	typedef AxisAlignedCube<EuclideanSpaceType> SecondType;
    bool operator()(const FirstType& a, const SecondType& b) const {
        for (size_t i = 0; i < EuclideanSpaceType::dimensionality();  ++i) {
            // If the minimum of a is greater than the maximum of b along one axis,
            // then they can not intersect.
            if (a.getMin()[i] > b.getMax()[i]) {
                return false;
            }
            // If the maximum of a is smaller than the minimum of b along one axis,
            // then they can not intersect.
            if (a.getMax()[i] < b.getMin()[i]) {
                return false;
            }
        }
        return true;
    }
}; // struct Intersects

} // namespace Math
} // namespace Ego
