#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

/**
 * @brief Functor defining an operator() which returns the zero value of type @a Type.
 * Specializations for all arithmetic types are provided.
 * @tparam Type the type
 */
template <typename Type, typename Enabled = void>
struct Zero;

template <typename Type>
struct Zero<Type, std::enable_if_t<std::is_same<float, Type>::value>> {
    Type operator()() const {
        return 0.0f;
    }
};
template <typename Type>
struct Zero<Type, std::enable_if_t<std::is_same<double, Type>::value>> {
    Type operator()() const {
        return 0.0;
    }
};

template <typename Type>
struct Zero<Type, std::enable_if_t<std::is_same<short, Type>::value>> {
    Type operator()() const {
        return 0;
    }
};

template <typename Type>
struct Zero<Type, std::enable_if_t<std::is_same<int, Type>::value>> {
    Type operator()() const {
        return 0;
    }
};

template <typename Type>
struct Zero<Type, std::enable_if_t<std::is_same<long, Type>::value>> {
    Type operator()() const {
        return 0l;
    }
};

template <typename Type>
struct Zero<Type, std::enable_if_t<std::is_same<long long, Type>::value>> {
    Type operator()() const {
        return 0ll;
    }
};

} // namespace Math
} // namespace Ego
