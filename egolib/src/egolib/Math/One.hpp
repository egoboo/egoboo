#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

/**
 * @brief Functor defining an operator() which returns the one value of type @a Type.
 * Specializations for all arithmetic types are provided.
 * @tparam Type the type
 */
template <typename Type, typename Enabled = void>
struct One;

template <typename Type>
struct One<Type, std::enable_if_t<std::is_same<float, Type>::value>> {
    Type operator()() const {
        return 1.0f;
    }
};

template <typename Type>
struct One<Type, std::enable_if_t<std::is_same<double, Type>::value>> {
    Type operator()() const {
        return 1.0;
    }
};

template <typename Type>
struct One<Type, std::enable_if_t<std::is_same<short, Type>::value>> {
    Type operator()() const {
        return 1.0;
    }
};

template <typename Type>
struct One<Type, std::enable_if_t<std::is_same<int, Type>::value>> {
    Type operator()() const {
        return 1.0;
    }
};

template <typename Type>
struct One<Type, std::enable_if_t<std::is_same<long, Type>::value>> {
    Type operator()() const {
        return 1l;
    }
};

template <typename Type>
struct One<Type, std::enable_if_t<std::is_same<long long, Type>::value>> {
    Type operator()() const {
        return 1ll;
    }
};

} // namespace Math
} // namespace Ego
