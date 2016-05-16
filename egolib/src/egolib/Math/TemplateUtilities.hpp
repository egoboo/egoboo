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
namespace Core {

/**@{*/
/**
 * @brief
 *  The following templates obtain the relation of two @a size_t compile-time constants.
 * @todo
 *  Make more general and simplify.
 */

/** @a std::true_type if <tt>n == m</tt>, @a std::false_type otherwise */
template <size_t n, size_t m>
using EqualTo = std::conditional_t<(n == m), std::true_type, std::false_type>;

/** @a std::true_type if <tt>n != m</tt>, @a std::false_type otherwise. */
template <size_t n, size_t m>
using NotEqualTo = std::conditional_t<(n != m), std::true_type, std::false_type>;

/** @a std::true_type if <tt>n > m</tt>, @a std::false_type otherwise */
template <size_t n, size_t m>
using GreaterThan = std::conditional_t<(n > m), std::true_type, std::false_type>;

/** @ astd::true_type if <tt>n >= m</tt>, @a std::false_type_otherwise */
template <size_t n, size_t m>
using GreaterThanOrEqualTo = std::conditional_t<(n >= m), std::true_type, std::false_type>;

/** @ std::true_type if <tt>n < m</tt>, @a std::false_type otherwise. */
template <size_t n, size_t m>
using LowerThan = std::conditional_t<(n < m), std::true_type, std::false_type>;

/** @a std::true_type if <tt>n <= m</tt>, @a std::false_type otherwise. */
template<size_t n, size_t m>
using LowerThanOrEqualTo = std::conditional_t<(n <= m), std::true_type, std::false_type>;

/**@}*/

} // namespace Core
} // namespace Ego

namespace Ego {
namespace Math {
namespace Internal {

template <size_t I, typename Type, size_t Size>
void unpack(Type(&dst)[Size]);

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

} // namespace Internal
} // namespace Math
} // namespace Ego

