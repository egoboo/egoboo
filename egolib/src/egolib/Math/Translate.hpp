#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

/// Translate functor.
/// Specializations for all types defined over Euclidean Space types (and Ego::Math::Point) are provided.
template <typename ... T>
struct Translate;

} // namespace Math
} // namespace Ego