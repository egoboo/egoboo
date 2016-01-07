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

/// @file   egolib/Math/Rings.hpp
/// @brief  Rings and commutative unitary rings.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

namespace Internal {
/**
 * @tparam _ElementType the element type
 * @tparam _Unital @a true if the ring is a unital ring
 * @tparam _Commutative @a true if the ring is commutative
 */
template <typename _ElementType, bool _Unital, bool _Commutative>
struct OrderedRing {

	/**
	 *  @brief
	 *  The type of an element of the set.
	 */
	typedef _ElementType ElementType;

	/**
	 * @brief
	 *	Is the ring unital?
	 */
	typedef std::integral_constant<bool, _Unital> IsUnital;

	/**
	 * @brief
	 *	Is the ring commutative?
	 */
	typedef std::integral_constant<bool, _Commutative> IsCommutative;

	/**
	 * @brief
	 *  Addition.
	 * @param x
	 *  the augend
	 * @param y
	 *  the addend
	 * @return
	 *	the sum <tt>x + y</tt>
	 */
	static inline ElementType sum(const ElementType& x, const ElementType& y) {
		return x + y;
	}

	/**
	 * @brief
	 *  Multiplication.
	 * @param x
	 *  the multiplier
	 * @param y
	 *  the multiplicand
	 * @return
	 *  the product <tt>x * y</tt>
	 */
	static inline ElementType product(const ElementType& x, const ElementType& y) {
		return x * y;
	}

	/**
	 * @brief
	 *  Subtraction.
	 * @param x
	 *  the minuend
	 * @param y
	 *  the subtrahend
	 * @return
	 *  the difference <tt>x - y</tt>
	 */
	static inline ElementType difference(ElementType x, ElementType y) {
		return x - y;
	}

    /**
     * @brief
     *  The additive inverse of an element.
     * @param x
     *  the element
     * @return
     *  the additive inverse of the element
     */
    static inline ElementType additiveInverse(const ElementType& x) {
        return -x;
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
	 * @remark
	 *	Only available if the element type is a floating-point type.
	 */
	static inline typename std::enable_if<std::is_floating_point<ElementType>::value, bool>::type equalToULP(const ElementType& x, const ElementType& y, const size_t ulp) {
		// The machine epsilon has to be scaled to the magnitude of the values used
		// and multiplied by the desired precision in ULPs (units in the last place).
		return std::abs(x - y) <= std::numeric_limits<ElementType>::epsilon() * std::abs(x + y) * ulp
			// Unless the result is subnormal
			|| std::abs(x - y) <= std::numeric_limits<ElementType>::min();
	}

	static inline std::enable_if_t<std::is_floating_point<ElementType>::value, bool> equalToTolerance(const ElementType& x, const ElementType& y, const ElementType& tolerance) {
		return std::abs(x - y) < tolerance;
	}

	/**
	 * @brief
	 *  "not equal to"
	 * @param x, y
	 *  the elements
	 * @param ulp
	 *  desired precision in ULPs (units in the last place)
	 * @return
	 *  @a true if <tt>x == y</tt>, @a false otherwise
	 * @remark
	 *	Only available if the element type is a floating-point type.
	 */
	static inline std::enable_if_t<std::is_floating_point<ElementType>::value, bool> notEqualToULP(const ElementType& x, const ElementType& y, const size_t ulp) {
		return !equalToULP(x, y, ulp);
	}

	static inline std::enable_if_t<std::is_floating_point<ElementType>::value, bool> notEqualToTolerance(const ElementType& x, const ElementType& y, const ElementType& tolerance) {
		return !equalToTolerance(x, y, tolerance);
	}

	/**
	 * @brief
	 *  "not equal to".
	 * @param x, y
	 *  the scalars
	 * @return
	 *  @a true if <tt>x &#8800; y</tt>, @a false otherwise
	 */
	static inline bool notEqualTo(const ElementType& x, const ElementType& y) {
		return x != y;
	}

	/**
	 * @brief
	 *  "equal to".
	 * @param x, y
	 *  the scalars
	 * @return
	 *  @a true if <tt>x == y</tt>, @a false otherwise
	 */
	static inline bool equalTo(const ElementType& x, const ElementType& y) {
		return x == y;
	}

	/**
	 * @brief
	 *  "lower than".
	 * @param x, y
	 *  the elements
	 * @return
	 *  @a true if <tt>x &lt; y</tt>, @a false otherwise
	 */
	static inline bool lowerThan(const ElementType& x, const ElementType& y) {
		return x < y;
	}

	/**
	 * @brief
	 *  "lower than or equal to".
	 * @param x, y
	 *  the elements
	 * @return
	 *  @a true if <tt>x &le; y</tt>, @a false otherwise
	 */
	static inline bool lowerThanOrEqualTo(const ElementType& x, const ElementType& y) {
		return x <= y;
	}

	/**
	 * @brief
	 *  "greater than".
	 * @param x, y
	 *  the elements
	 * @return
	 *  @a true if <tt>x &gt; y</tt>, @a false otherwise
	 */
	static inline bool greaterThan(const ElementType& x, const ElementType& y) {
		return x > y;
	}

	/**
	 * @brief
	 *  "greater than or equal to".
	 * @param x, y
	 *  the elements
	 * @return
	 *  @a true if <tt>x &ge; y</tt>, @a false otherwise
	 */
	static inline bool greaterThanOrEqualTo(const ElementType& x, const ElementType& y) {
		return x >= y;
	}

};

} // namespace Internal

/**
 * @brief
 *	A ring \f$(R, +, \cdot)\f$ is a set \f$R\f$ equipped with two binary operations
 *	\f$+: R \times R \rightarrow R\f$ and \f$\cdot : R \times R \rightarrow R\f$ with the following properties:
 *	<table>
 *		<tr><td>Closure: </td>                         <td>\f$a + b \in R\f$</td>                                               <td>\f$a \cdot b \in R\f$                          </td></tr>
 *		<tr><td>Commutativity: </td>                   <td>\f$a + b = b + a\f$</td>				                                <td>                                               </td></tr>
 *		<tr><td>Associativity: </td>                   <td>\f$a + (b + c) = (a + b) + c\f$</td>                                 <td>\f$a \cdot (b \cdot c) = (a \cdot b) \cdot c\f$</td></tr>
 *		<tr><td>Existence of an identity element:</td> <td>\f$a + 0 = a\f$</td>                                                 <td>                                               </td></tr>
 *		<tr><td>Existence of an inverse element:</td>  <td>\f$\forall a \in R : \exists -a \in R : a + (-a) = 0\f$</td>         <td>                                               </td></tr>
 *	    <tr><td>Distributivity of multiplication over addition:</td> <td>\f$a \cdot (b + c) = (a \cdot b) + (a \cdot c)\f$</td> <td>                                               </td></tr>
 *	< / table>
 *	\f$+\f$ and \f$\cdot\f$ are commonly called addition and multiplication, \f$0\f$ is commonly called the zero / additive neutral element / neutral element of addition.
 * @remark
 *	An ordering \f$(R, <:)\f$ is called compatible with the ring structure if
 *	- \f$<:\f$ is compatible with \f$+\f$ and
 *	- \f$\forall a, b \in R : 0 < a \wedge 0 < b \Rightarrow 0 < (a \circ b)\f$
 *	A relation \f$r : R \times R\f$ is called compatible with a binary operation \f$o : R \times R \rightarrow R\f$ if
 *	- \f$\forall a, b, c \in R: a r b \Rightarrow (a o c) r (b o c)\f$
 *	- \f$\forall a, b, c \in R: a r b \Rightarrow (c o a) r (c o b)\f$
 * @remark
 *	An ordered ring \f$(R, +, \cdot, \leq)\f$ is a ring \f$(R, +, \cdot)\f$ is a ring with an ordering \f$\leq\f$ that is compatible with the ring structure.
 *	Ordered rings are familiar from arithmetic. Examples include the integers, the rationals and the real numbers. (The rationals and reals in fact form ordered fields.)
 *	The complex numbers, in contrast, do not form an ordered ring or field, because there is no inherent order relationship between the elements \f$1\f$ and \f$i\f$.
 * @remark
 *	In analogy with the real numbers, we call an element \f$c \neq 0\f$ of an ordered ring positive if \f$0 \leq c\f$, and negative if \f$c \leq 0\f$.
 *	The element \f$c = 0\f$ is considered to be neither positive nor negative.
 * @remark
 *	A commutative ring is a ring \f$(R, +, \cdot)\f$ for which the property
 *	<table>
 *		<tr><td>Commutativity:</td><td>\f$a \cdot b = b \cdot a\f$</td></tr>
 *	</table>
 *	holds.
 * @remark
 *	An unital ring ring is a ring \f$(R, +, \cdot)\f$ for which the property
 *	<table>
 *		<tr><td>Commutativity: </td><td>\f$a + b = b + a\f$</td></tr>
 *	</table>
 *	holds.
 */
template <typename _ElementType, typename _Enabled = void>
struct OrderedRing;

template <typename _ElementType>
struct OrderedRing<_ElementType, std::enable_if_t<std::is_integral<_ElementType>::value && std::is_signed<_ElementType>::value && !std::is_const<_ElementType>::value>>
	: public Internal::OrderedRing<_ElementType, true, true> {
public:
	typedef typename Internal::OrderedRing<_ElementType, true, true>::ElementType ElementType;

	/**
	 * @brief
	 *  The zero/neutral element of addition/additive neutral element.
	 * @return
	 *  the zero/neutral element of addition/additive neutral element
	 */
	static inline constexpr ElementType additiveNeutral() {
		return 0;
	}

	/**
	 * @brief
	 *  The multiplicative neutral element.
	 * @return
	 *  the multiplicative neutral element
	 */
	static inline constexpr ElementType multiplicativeNeutral() {
		return 1;
	}

public:
    /**
     * @brief
     *  "is negative"
     * @param x
     *  the scalar
     * @return @a true if <tt>x &lt; 0</tt>,
     *  @a false otherwise
     */
    static inline bool isNegative(const ElementType& x) {
        return x < additiveNeutral();
    }

    /**
     * @brief
     *  "is positive"
     * @param x
     *  the scalar
     * @return
     *  @a true if <tt>x &gt; 0</tt>, @a false otherwise
     */
    static inline bool isPositive(const ElementType& x) {
        return x > additiveNeutral();
    }

};

template <typename _ElementType>
struct OrderedRing<_ElementType, std::enable_if_t<std::is_same<_ElementType, float>::value>>
	: public Internal::OrderedRing<_ElementType, true, true> {
public:
	typedef typename Internal::OrderedRing<_ElementType, true, true>::ElementType ElementType;

	/**
	 * @brief
	 *  The zero/neutral element of addition/additive neutral element.
	 * @return
	 *  the zero/neutral element of addition/additive neutral element
	 */
	static inline constexpr ElementType additiveNeutral() {
		return 0.0f;
	}

	/**
	 * @brief
	 *  The multiplicative neutral element.
	 * @return
	 *  the multiplicative neutral element
	 */
	static inline constexpr ElementType multiplicativeNeutral() {
		return 1.0f;
	}

public:
    /**
     * @brief
     *  "is negative"
     * @param x
     *  the scalar
     *  @return @a true if <tt>x &lt; 0</tt>,
     *  @a false otherwise
     */
    static inline bool isNegative(const ElementType& x) {
        return x < additiveNeutral();
    }

    /**
     * @brief
     *  "is positive"
     * @param x
     *  the scalar
     * @return
     *  @a true if <tt>x &gt; 0</tt>, @a false otherwise
     */
    static inline bool isPositive(const ElementType& x) {
        return x > additiveNeutral();
    }

};

template <typename _ElementType>
struct OrderedRing<_ElementType, std::enable_if_t<std::is_same<_ElementType, double>::value>>
	: public Internal::OrderedRing<_ElementType, true, true> {
public:
	typedef typename Internal::OrderedRing<_ElementType, true, true>::ElementType ElementType;

	/**
	 * @brief
	 *  The zero/neutral element of addition/additive neutral element.
	 * @return
	 *  the zero/neutral element of addition/additive neutral element
	 */
	static inline constexpr ElementType additiveNeutral() {
		return 0.0;
	}

	/**
	 * @brief
	 *  The multiplicative neutral element.
	 * @return
	 *  the multiplicative neutral element
	 */
	static inline constexpr ElementType multiplicativeNeutral() {
		return 1.0;
	}

public:
    /**
     * @brief
     *  "is negative"
     * @param x
     *  the scalar
     * @return @a true if <tt>x &lt; 0</tt>,
     *  @a false otherwise
     */
    static inline bool isNegative(const ElementType& x) {
        return x < additiveNeutral();
    }

    /**
     * @brief
     *  "is positive"
     * @param x
     *  the scalar
     * @return
     *  @a true if <tt>x &gt; 0</tt>, @a false otherwise
     */
    static inline bool isPositive(const ElementType& x) {
        return x > additiveNeutral();
    }

};

} // namespace Math
} // namespace Ego
