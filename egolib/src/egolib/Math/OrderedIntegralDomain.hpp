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

/// @file   egolib/Math/OrderedIntegralDomain.hpp
/// @brief  Integral domain.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/OrderedRing.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  An ordered integral domain (based on the mathematical ideal of an "integral domain": http://mathworld.wolfram.com/IntegralDomain.html).
 *	An ordered integral domain subsumes properties of the integers, in fact, the motiviation for the definition of an (ordered) integral
 *	domain is the set of integers. An ordered integral domain I is an ordered ring that fulfils the following additional properties:
 *	<table>
 *		<tr><td>                       <td>Addition</td> <td>Multiplication</td></tr>
 *		<tr><td>No zero divisors:</td> <td></td>         <td>\f$a \cdot b = 0 \Rightarrow a = 0 \vee b = 0\f$</td></tr>
 *	</table>
 * @remark
 *	The rational numbers and real numbers are ordered integral domains as well, however, they have additional properties, such that they
 *	are in fact "ordered fields".
 * @author
 *  Michael Heilmann
 */
template <typename _ElementType, typename _Enabled = void>
struct OrderedIntegralDomain;

template <typename _ElementType>
struct OrderedIntegralDomain<_ElementType, 
	typename std::enable_if<
		(std::is_integral<_ElementType>::value && std::is_signed<_ElementType>::value && !std::is_const<_ElementType>::value) ||
		std::is_floating_point<_ElementType>::value>::type>
	: public OrderedRing<_ElementType> {
	
	/**
	 * @brief
	 *  The type of an element of the set.
	 */
	typedef typename OrderedRing<_ElementType>::ElementType ElementType;

};

} // namespace Math
} // namespace Ego
