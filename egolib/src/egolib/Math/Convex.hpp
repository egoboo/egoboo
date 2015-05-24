#pragma once

#include "egolib/Math/AABB.hpp"
#include "egolib/Math/Sphere.h"

namespace Ego {
namespace Math {

/**
 * @brief
 *  Compute the smallest volume enclosing a given volume.
 * @param source
 *  the source volume
 * @param target
 *  the target volume
 */
template <typename SourceType, typename TargetType>
TargetType convexHull(const SourceType& source);

template <>
inline sphere_t convexHull(const aabb_t& source) {
    const auto center = source.getCenter();
    const auto radius = (source.getMin() - center).length();
    return sphere_t(center, radius);
}


} // namespace Math
} // namespace Ego
