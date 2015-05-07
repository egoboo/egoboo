#pragma once

#include "egolib/platform.h"

namespace Ego
{

namespace Core
{

/**@{*/
/**
 * @brief
 *  The following templates indicate the relation of two @a size_t
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


} // namespace Core

namespace Math
{

} // namespace Math

} // namespace Ego