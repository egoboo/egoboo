#pragma once

#include "egolib/Math/Scalar.hpp"
#include "egolib/Math/OrderedIntegralDomain.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  An field (based on the mathematical ideal of a "field": http://mathworld.wolfram.com/Field.html).
 *	A field subsumes essential properties of arithmetic types. A field F is an integral domain such
 *	that any non-zero element a != 0 has a multiplicative inverse a^-1 such that a a^-1 = 1. This
 *	way, division can be defined as a / b = a b^-1 where the denominator b must be non-zero.
 * @author
 *  Michael Heilmann
 * @remark
 *  Implementations for floating point types (float and double) are provided.
 * @todo
 *  Provide an implementation for the restrictions of floating point types
 *  (float and double) to the interval [0,1] as well.
 * @author
 *  Michael Heilmann
 */
template <typename _ScalarType, typename _Enabled = void>
struct Field;

template <typename _ScalarType>
struct Field<_ScalarType, typename std::enable_if<IsScalar<_ScalarType>::value>::type>
	: public OrderedIntegralDomain<_ScalarType>{

    /**
     * @brief
     *  The scalar type.
     */
    typedef _ScalarType ScalarType;

	/**
	 * @invariant
	 *  The scalar type must be a floating point type.
	 */
	static_assert(IsScalar<_ScalarType>::value, "_ScalarType must fulfil the scalar concept");
    
public:
   
    /** 
     * @brief
     *  "division"
     * @param x
     *  the divident
     * @param y
     *  the divisor
     * @return
     *  the quotient <tt>x / y</tt>
     */
    static inline ScalarType quotient(const ScalarType& x, const ScalarType& y) {
        return x / y;
    }
 
};

} // namespace Math
} // namespace Ego
