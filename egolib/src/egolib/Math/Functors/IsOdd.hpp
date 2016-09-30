#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

/// @brief Functor which determines wether an integral type is even or odd.
template <typename Type, typename Enabled = void>
struct IsOdd;

template <typename Type>
struct IsOdd<Type, std::enable_if_t<std::is_integral<Type>::value>> {
    bool operator()(const Type& x) const {
        return 1 == (x & 1);
    }
};

} // namespace Math
} // namespace Ego
