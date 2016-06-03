#pragma once

namespace Ego {
namespace Math {
/**
 * @brief Determine wether two entities intersect.
 * @tparam the Euclidean space type to which both entities belong
 * @tparam the type of the first entity
 * @tparam the type of the second entity
 * @remark The intersects relation is commutative.
 */
template <typename ... T>
struct Intersects;
} // namespace Math
} // namespace Ego
