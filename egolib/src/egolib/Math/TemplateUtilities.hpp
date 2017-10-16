//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file   egolib/Math/TemplateUtilities.h
/// @brief  Miscellaneous utilities for template metaprogramming
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {

namespace Math {

template <typename T>
struct IsReal {
    static constexpr bool value =
        std::is_floating_point<T>::value;
};

template <typename T>
struct IsInteger {
    static constexpr bool value =
        std::is_same<short, T>::value ||
        std::is_same<int, T>::value ||
        std::is_same<long, T>::value ||
        std::is_same<long long, T>::value;
};

}

namespace Core {

/**
 * @brief
 *  The following templates obtain the relation of one the boolean values in a variadic template argument list
 *  @a size_t compile-time constants.
 * @todo
 *  Make more general and simplify.
 */
template <bool...> struct BoolPack;

/**
 * @brief
 *  Provide a member constant value <tt>value</tt>.
 *  That constant is equal to <tt>true</tt> if
 *  all boolean values in the variadic template argument list are true.
 *  Otherwise it is equal to <tt>false</tt>.
 */
template <bool... v>
using AllTrue  = std::is_same<BoolPack<true, v...>, BoolPack<v..., true>>;

/**
 * @brief
 *  Provide a mamber constant value <tt>value</tt>.
 *  That constant is equal to <tt>true</tt> if
 *  all boolean values in the variadic template argument list are true.
 *  Otherwise it is equal to <tt>false</tt>.
 */
template <bool... v>
using AllFalse = std::is_same<BoolPack<false, v...>, BoolPack<v..., false>>;

/** 
 * @brief
 *  Provide member constant value <tt>value</tt>.
 *  That constant is equal to <tt>true</tt>
 *  if the argument types in the variadic template argument list are convertible into the specified target type.
 *  Otherwise it is equal to <tt>false</tt>.
 */
template <typename TargetType, typename ... ArgumentTypes>
using AllConvertible = AllTrue<std::is_convertible<ArgumentTypes, TargetType>::value ...>;

} // namespace Core
} // namespace Ego

namespace Ego {
namespace Math {
namespace Internal {

template <size_t I, typename  Type, size_t Size, typename Arg>
void _unpack(Type(&dst)[Size], Arg&& arg) {
	dst[I] = arg;
}

template <size_t I, typename Type, size_t Size, typename Arg, class ... Args>
void _unpack(Type(&dst)[Size], Arg&& arg, Args&& ... args) {
	dst[I] = arg;
	_unpack<I + 1, Type>(dst, args ...);
}

/**
* @brief
*  You can use "unpack" for storing non-type parameter packs in arrays.
* @invariant
*  The number of arguments must be exactly the length of the array.
* @invariant
*  The argument types must be the array type.
* @remark
*  Example usage:
*  @code
*  float a[3]; unpack(a,0.5,0.1)
*  @endcode
* @author
*  Michael Heilmann
*/
template <typename Type, size_t Size, typename ... Args>
void unpack(Type(&dst)[Size], Args&& ...args) {
	static_assert(Size == sizeof ... (args), "wrong number of arguments");
	_unpack<0, Type, Size>(dst, args ...);
}

template <typename VectorType, typename ScalarType>
struct VectorExpr : public id::equal_to_expr<VectorType>,
                    public id::plus_expr<VectorType>,
                    public id::minus_expr<VectorType>,
                    public id::unary_plus_expr<VectorType>,
                    public id::unary_minus_expr<VectorType>
{
    VectorType& operator*=(const ScalarType& scalar)
    {
        static_cast<VectorType *>(this)->multiply(scalar);
        return *static_cast<VectorType *>(this);
    }

    // friends defined inside class body are inline and are hidden from non-ADL lookup
    friend VectorType operator*(VectorType lhs,        // passing lhs by value helps optimize chained a+b+c
                                const ScalarType& rhs) // otherwise, both parameters may be const references
    {
        lhs *= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

    VectorType& operator/=(const ScalarType& scalar)
    {
        static_cast<VectorType *>(this)->divide(scalar);
        return *static_cast<VectorType *>(this);
    }

    // friends defined inside class body are inline and are hidden from non-ADL lookup
    friend VectorType operator/(VectorType lhs,        // passing lhs by value helps optimize chained a+b+c
                                const ScalarType& rhs) // otherwise, both parameters may be const references
    {
        lhs /= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }
};

template <typename PointType, typename VectorType>
struct PointExpr : public id::equal_to_expr<PointType>
{
    friend VectorType operator-(const PointType& a, const PointType& b)
    {
        return static_cast<const PointType *>(&a)->difference(b);
    }

    PointType& operator+=(const VectorType& rhs)
    {
        static_cast<PointType *>(this)->translate(rhs);
        return *static_cast<PointType *>(this);
    }
    // friends defined inside class body are inline and are hidden from non-ADL lookup
    friend PointType operator+(PointType lhs,        // passing lhs by value helps optimize chained a+b+c
                               const VectorType& rhs) // otherwise, both parameters may be const references
    {
        lhs += rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

    PointType& operator-=(const VectorType& rhs) // compound assignment (does not need to be a member,
    {                                 // but often is, to modify the private members)
       
        (*static_cast<PointType *>(this)) += -rhs;
        return *static_cast<PointType *>(this); // return the result by reference
    }
    // friends defined inside class body are inline and are hidden from non-ADL lookup
    friend PointType operator-(PointType lhs,        // passing lhs by value helps optimize chained a+b+c
                               const VectorType& rhs) // otherwise, both parameters may be const references
    {
        lhs -= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }
};

} // namespace Internal
} // namespace Math
} // namespace Ego

