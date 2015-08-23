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

/// @file   egolib/Math/VectorSpace.hpp
/// @brief  A vector space.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Dimensionality.hpp"
#include "egolib/Math/OrderedField.hpp"

namespace Ego {
namespace Math {

namespace Internal {

/**
 * @brief
 *  A struct derived from @a std::true_type if @a _Dimensionality fulfils the
 *  properties of a <em>dimensionality  concept</em> and derived from
 *  @a std::false_type otherwise. Furthermore, @a _ScalarFieldType shall be a
 *	be of type <tt>Ego::Math::ScalarField</tt>.
 */
template <typename _ScalarFieldType, size_t _Dimensionality>
struct VectorSpaceEnable
	: public std::conditional <
	  (IsDimensionality<_Dimensionality>::value),
	  std::true_type,
	  std::false_type
	  >::type
{};


} // namespace Internal

/**
 * @brief
 *	A vector space.
 * @param _ScalarFieldType
 *	the underlaying type. Must fulfil the scalar field concept.
 *	A line.
 * @todo
 *	Rename @a _ScalarFieldType to _UnderlayingType.
 * @author
 *	Michael Heilmann
 */
template <typename _ScalarFieldType, size_t _Dimensionality, 
		  typename _Enabled = void>
struct VectorSpace;

template <typename _ScalarFieldType, size_t _Dimensionality>
struct VectorSpace<_ScalarFieldType, _Dimensionality, 
	               typename std::enable_if<Internal::VectorSpaceEnable<_ScalarFieldType, _Dimensionality>::value>::type> {
	
	/**
	 * @invariant
	 *	@a _Dimensionality must fulfil the <em>dimensionality concept</em>.
	 */
	static_assert(IsDimensionality<_Dimensionality>::value, "_Dimensionality must fulfil the dimensionality concept");

	/**
	 * @brief
	 *	The scalar type.
	 */
	typedef typename _ScalarFieldType::ScalarType ScalarType;
	/**
	 * @brief
	 *	The scalar field type.
	 */
	typedef _ScalarFieldType ScalarFieldType;

#if !defined(_MSC_VER)
	/**
	 * @brief
	 *	The dimensionality of the vector space.
	 * @return
	 *	the dimensionality of the vector space
	 */
	constexpr static size_t dimensionality() {
		return _Dimensionality;
	}
#endif

	/**
	 * @brief
	 *	The dimensionality of the vector space.
	 * @todo
	 *	Should be a constexpr function. Not yet possible because of - guess what - MSVC.
	 */
	typedef typename std::integral_constant < size_t, _Dimensionality > Dimensionality;
};

} // namespace Math
} // namespace Ego
