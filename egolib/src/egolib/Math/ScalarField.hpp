#pragma once

#include "egolib/Math/Scalar.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  Describes a scalar field, in particular, provides relational and arithmetic operators for the scalars.
 *
 *  Implementations for floating point types (float and double) are provided.
 * @todo
 *  Provide an implementation for the restricitions of floating point types
 *  (float and double) to the interval [0,1] as well.
 * @author
 *  Michael Heilmann
 */
template <typename _ScalarType, typename _Enabled = void>
struct ScalarField;

template <typename _ScalarType>
struct ScalarField<_ScalarType, typename std::enable_if<IsScalar<_ScalarType>::value>::type> {

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
     *  The multiplicative neutral element.
     * @return
     *  the multiplicative neutral element
     */
    static inline ScalarType muliplicativeNeutral() {
        return 1.0;
    }

    /**
     * @brief
     *  The additive neutral element.
     * @return
     *  the additive neutral element
     */
    static inline ScalarType additiveNeutral() {
        return 0.0;
    }
    
public:

    /** 
     * @brief
     *  "equal to".
     * @param x, y
     *  the scalars
     * @return
     *  @a true if <tt>x == y</tt>, @a false otherwise
     */
    static inline bool equalTo(const ScalarType& x, const ScalarType& y) {
        return x == y;
    }
    /** 
     * @brief
     *  "equal to"
     * @param x, y
     *  the scalars
     * @param ulp
     *  desired precision in ULPs (units in the last place)
     * @return
     *  @a true if <tt>x == y</tt>, @a false otherwise
     * @remark
     *  Just taken from http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
     *  and those guys kinda collected that from Knuth and 1000ends of other articles.
     *  
     */
    static inline bool equalToULP(const ScalarType& x, const ScalarType& y, const size_t ulp) {
        // The machine epsilon has to be scaled to the magnitude of the values used
        // and multiplied by the desired precision in ULPs (units in the last place).
        return std::abs(x-y) <= std::numeric_limits<ScalarType>::epsilon() * std::abs(x+y) * ulp
        // Unless the result is subnormal
               || std::abs(x-y) <= std::numeric_limits<ScalarType>::min();
    }
    static inline bool equalToTolerance(const ScalarType& x, const ScalarType& y, const ScalarType& tolerance) {
        return std::abs(x - y) < tolerance;
    }
     
    /** 
     * @brief
     *  "not equal to".
     * @param x, y
     *  the scalars
     * @return
     *  @a true if <tt>x &#8800; y</tt>, @a false otherwise
     */
    static inline bool notEqualTo(const ScalarType& x, const ScalarType& y) {
        return x !=  y;
    }
    /** 
     * @brief
     *  "not equal to"
     * @param x, y
     *  the scalars
     * @param ulp
     *  desired precision in ULPs (units in the last place)
     * @return
     *  @a true if <tt>x == y</tt>, @a false otherwise
     */
    static inline bool notEqualToULP(const ScalarType& x, const ScalarType& y, const size_t ulp) {
        return !equalToULP(x,y,ulp);
    }
    static inline bool notEqualToTolerance(const ScalarType& x, const ScalarType& y, const ScalarType& tolerance) {
        return !equalToTolerance(x, y, tolerance);
    }
    
public:
    
    /**
     * @brief
     *  "lower than".
     * @param x, y
     *  the scalars
     * @return
     *  @a true if <tt>x &lt; y</tt>, @a false otherwise
     */
    static inline bool lowerThan(const ScalarType& x, const ScalarType& y) {
        return x < y;
    }

    /**
     * @brief
     *  "lower than or equal to".
     * @param x, y
     *  the scalars
     * @return
     *  @a true if <tt>x &le; y</tt>, @a false otherwise
     */
    static inline bool lowerThanOrEqualTo(const ScalarType& x, const ScalarType& y) {
        return x <= y;
    }

    /**
     * @brief
     *  "greater than".
     * @param x, y
     *  the scalars
     * @return
     *  @a true if <tt>x &gt; y</tt>, @a false otherwise
     */
     static inline bool greaterThan(const ScalarType& x, const ScalarType& y) {
        return x > y;
     }

    /** 
     * @brief
     *  "greater than or equal to".
     * @param x, y
     *  the scalars
     * @return
     *  @a true if <tt>x &ge; y</tt>, @a false otherwise
     */
     static inline bool greaterThanOrEqualTo(const ScalarType& x, const ScalarType& y) {
        return x >= y;
     }
     
public:

    /**
     * @brief
     *  "addition"
     * @param x
     *  the augend
     * @param y
     *  the addend
     * @return
     *  the sum <tt>x + y</tt>
     */
    static inline ScalarType sum(const ScalarType& x, const ScalarType& y) {
        return x + y;
    }

    /**
     * @brief
     *  "subtraction"
     * @param x
     *  the minuend
     * @param y
     *  the subtrahend
     * @return
     *  the difference <tt>x - y</tt>
     */
    static inline ScalarType difference(const ScalarType& x, const ScalarType& y) {
        return x - y;
    }
    
    /**
     * @brief
     *  "multiplication"
     * @param x
     *  the multiplier
     * @param y
     *  the multiplicand
     * @return
     *  the product <tt>x * y</tt>
     */
    static inline ScalarType product(const ScalarType& x, const ScalarType& y) {
        return x * y;
    }
    
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
    
public:

    /** 
     * @brief
     * "is negative"
     * @param x
     *  the scalar
     * @return @a true if <tt>x &lt; 0</tt>,
     *  @a false otherwise
     */
    static inline bool isNegative(const ScalarType& x) {
        return x < ScalarField<ScalarType>::additiveNeutral();
    }

    /** 
     * @brief
     *  "is positive"
     * @param x
     *  the scalar
     * @return
     *  @a true if <tt>x &gt; 0</tt>, @a false otherwise
     */
    static inline bool isPositive(const ScalarType& x) {
        return x > ScalarField<ScalarType>::additiveNeutral();
    }
 
};

} // namespace Math
} // namespace Ego