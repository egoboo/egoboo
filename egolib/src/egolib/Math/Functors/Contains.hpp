#pragma once

namespace Ego {
namespace Math {
/**
 * @brief Determine wether an entity is contained in another entity.
 * @tparam the Euclidean space type to which both entities belong
 * @tparam the type of the first entity
 * @tparam the type of the second entity
 * @remark The contains relation is <em>not necessarily</em> commutative.
 */
template <typename ... T>
struct Contains;
} // namespace Math
} // namespace Ego
