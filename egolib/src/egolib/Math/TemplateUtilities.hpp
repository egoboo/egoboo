#pragma once

#include "egolib/platform.h"

namespace Ego
{
namespace Core
{

/**@{*/
/**
 * @brief
 *  The following templates obtain  the relation of two @a size_t
 *  compile-time constants.
 * @todo
 *  Make more general and simplify.
 */
 

/** @a std::true_type if <tt>n == m</tt>, @a std::false_type otherwise */
template <size_t n, size_t m>
struct EqualTo
    : public std::conditional<(n == m), std::true_type, std::false_type>::type
{};

/** @a std::true_type if <tt>n != m</tt>, @a std::false_type otherwise. */
template <size_t n, size_t m>
struct NotEqualTo
    : public std::conditional<(n != m), std::true_type, std::false_type>::type
{};


/** @a std::true_type if <tt>n > m</tt>, @a std::false_type otherwise */
template <size_t n, size_t m>
struct GreaterThan
    : public std::conditional<(n > m), std::true_type, std::false_type >::type
{};

/** @ astd::true_type if <tt>n >= m</tt>, @a std::false_type_otherwise */
template <size_t n, size_t m>
struct GreaterThanOrEqualTo
    : public std::conditional<(n >= m), std::true_type, std::false_type>::type
{};

/** @ std::true_type if <tt>n < m</tt>, @a std::false_type otherwise. */
template <size_t n, size_t m>
struct LowerThan
    : public std::conditional<(n < m), std::true_type, std::false_type>::type
{};

/** @a std::true_type if <tt>n <= m</tt>, @a std::false_type otherwise. */
template <size_t n, size_t m>
struct LowerThanOrEqualTo
    : public std::conditional<(n <=  m), std::true_type, std::false_type>::type
{};


/**@}*/

/**@{*/
/**
 * @brief
 *  The following templates create an increasing sequence of consecutive size_t values.
 * @author
 *  Michael Heilmann
 * @todo
 *  Make more general and simplify.
 * @remark
 *  To illustrate how this works, we unroll the chain of subclass relations
 *  induced by the instantiation <tt>make_index_sequence<5></tt>. In the
 *  following listing (the "x" extends "y" relation is denoted by "x <: y"):
 *  @code
 *  make_index_sequence<5> <:
 *  <: make_index_sequence<4,4>
 *  <: make_index_sequence<3,3,4>
 *  <: make_index_sequence<2,2,3,4>
 *  <: make_index_sequence<1,1,2,3,4>
 *  <: make_index_sequence<0,0,1,2,3,4>
 *  <: index_sequence<0,1,2,3,4>
 *  @endcode
 */


/**
 * @brief
 *  The type of a sequence of indices.
 */
template<size_t ...>
struct index_sequence { };

/**
 * To demonstrate how the index sequence is generated (x <: y means "x extends y"),
 * @code
 * make_index_sequence<5> <:
 * <: make_index_sequence<4,4>
 * <: make_index_sequence<3,3,4>
 * <: make_index_sequence<2,2,3,4>
 * <: make_index_sequence<1,1,2,3,4>
 * <: make_index_sequence<0,0,1,2,3,4>
 * <: index_sequence<0,1,2,3,4>
 * @endcode
 */
template<size_t N, size_t ...S>
struct make_index_sequence : make_index_sequence<N - 1, N - 1, S...> { };

template<size_t ...S>
struct make_index_sequence<0, S...> : public index_sequence<S ...>
{};


/**@}*/

} // namespace Core
} // namespace Ego
