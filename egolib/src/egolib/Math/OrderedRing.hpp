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

#include "egolib/Math/TemplateUtilities.hpp"
#include "egolib/Math/Zero.hpp"
#include "egolib/Math/One.hpp"
#include "egolib/Float.hpp"

namespace Ego {
namespace Math {


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
 /**
  * @tparam _ElementType the element type
  */
template <typename _ElementType, typename _Enabled = void>
struct OrderedRing;

template <typename _ElementType>
struct OrderedRing<_ElementType, std::enable_if_t<IsInteger<_ElementType>::value || IsReal<_ElementType>::value>> {

	/**
	 *  @brief
	 *  The type of an element of the set.
	 */
	using ElementType = _ElementType;

	/**
	 * @brief
	 *	Is the ring unital?
	 */
    using IsUnital = std::integral_constant<bool, true>;

	/**
	 * @brief
	 *	Is the ring commutative?
	 */
    using IsCommutative = std::integral_constant<bool, true>;

    /**
     * @brief
     *  The zero/neutral element of addition/additive neutral element.
     * @return
     *  the zero/neutral element of addition/additive neutral element
     */
    static inline constexpr ElementType additiveNeutral() {
        return Zero<ElementType>()();
    }

    /**
     * @brief
     *  The multiplicative neutral element.
     * @return
     *  the multiplicative neutral element
     */
    static inline constexpr ElementType multiplicativeNeutral() {
        return One<ElementType>()();
    }

	/**
	 * @brief Functor computing the sum of two elements.
	 */
    struct SumFunctor {
        /**
         * @brief The result type.
         */
        using ResultType = ElementType;
        /**
         * @brief Compute the sum of two elements.
         * @param a the augend
         * @param b the addend
         * @return the sum <tt>a + b</tt>
         */
        ElementType operator()(const ElementType& a, const ElementType& b) const {
            return a + b;
        }
    };

	/**
	 * @brief Functor computing the product of two elements.
	 */
    struct ProductFunctor {
        /**
         * @brief The result type.
         */
        using ResultType = ElementType;
        /**
         * @brief Compute the product of two elements.
         * @param a the multiplier
         * @param b the multiplicand
         * @return the product <tt>a * b</tt>
         */
        ElementType operator()(const ElementType& a, const ElementType& b) const {
            return a * b;
        }
    };

	/**
	 * @brief Functor computing the difference of two elements.
	 */
    struct DifferenceFunctor {
        /**
         * @brief The result type.
         */
        using ResultType = ElementType;
        /**
         * @brief Compute the difference of two elements.
         * @param a the minuend
         * @param b the subtrahend
         * @return the difference <tt>a - b</tt>
         */
        ElementType operator()(const ElementType& a, const ElementType& b) const {
            return a - b;
        }
    };

    /// @brief Functor computing the unary minus of an element.
    struct UnaryMinusFunctor
    {
        /// @brief The result type.
        using ResultType = ElementType;
        /// @brief Compute the additive inverse of an element.
        /// @param a the element
        /// @return the unary minus <tt>-a</tt> of the element
        ElementType operator()(const ElementType& a) const
        {
            return -a;
        }
    };

    /// @brief Functor computing the unary plus of an element.
    struct UnaryPlusFunctor
    {
        /// @brief The result type.
        using ResultType = ElementType;
        /// @brief Compute the identity of an element.
        /// @param a the element
        /// @return the unary plus <tt>+a</tt> of the element
        ElementType operator()(const ElementType& a) const
        {
            return +a;
        }
    };

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
	 * @brief Functor computing if two elements are equal.
	 */
    struct EqualsFunctor {
        /**
         * @brief The result type.
         */
        using ResultType = bool;
        /**
         * @brief Compute if two elements are equal.
         * @param acc the accumulator variable
         * @param a, b the element
         * @return @a true if <tt>a == b</tt>, @a false otherwise
         */
        bool operator()(bool acc, const ElementType& a, const ElementType& b) const {
            return acc && (a == b);
        }
    };


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

public:
    /**
    * @brief
    *  "equal to"
    * @param x, y
    *  the scalars
    * @param ulp
    *  see float_equalToUlp
    * @return
    *  @a true if <tt>x ~ y</tt>, @a false otherwise
    * @remark
    *	Only available if the element type is a floating-point type.
    */
    static inline std::enable_if_t<std::is_floating_point<ElementType>::value, bool>
        equalToUlp(const ElementType& x, const ElementType& y, const size_t ulp) {
        return float_equalToUlp(x, y, ulp);
    }

    /**
    * @brief
    *  "equal to"
    * @param x, y
    *  the scalars
    * @param tolerance
    *  see float_equalToTolerance
    * @return
    *  @a true if <tt>x ~ y</tt>, @a false otherwise
    * @remark
    *	Only available if the element type is a floating-point type.
    */
    static inline std::enable_if_t<std::is_floating_point<ElementType>::value, bool>
        equalToTolerance(const ElementType& x, const ElementType& y, const ElementType& tolerance) {
        return float_equalToTolerance(x, y, tolerance);
    }

    /**
     * @brief
     *  "not equal to"
     * @param x, y
     *  the elements
     * @param ulp
     *  desired
     *  precision in ULPs (units in the last place) (size_t value)
     * @return
     *  @a true if <tt>x !~ y</tt>, @a false otherwise
     * @remark
     *	Only available if the element type is a floating-point type.
     */
    static inline std::enable_if_t<std::is_floating_point<ElementType>::value, bool>
        notEqualToUlp(const ElementType& x, const ElementType& y, const size_t ulp) {
        return !equalToUlp(x, y, ulp);
    }

    /**
     * @brief
     *  "not equal to"
     * @param x, y
     *  the scalars
     * @param tolerance
     *  desired
     *  tolerance (non-negative floating-point value)
     * @return
     *  @a true if <tt>x !~ y</tt>, @a false otherwise
     * @remark
     *	Only available if the element type is a floating-point type.
     */
    static inline std::enable_if_t<std::is_floating_point<ElementType>::value, bool>
        notEqualToTolerance(const ElementType& x, const ElementType& y, const ElementType& tolerance) {
        return !equalToTolerance(x, y, tolerance);
    }
};

} // namespace Math
} // namespace Ego
