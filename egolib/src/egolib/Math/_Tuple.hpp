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

/// @file   egolib/Math/_Tuple.hpp
/// @brief  Vectors.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/VectorSpace.hpp"
#include "egolib/log.h"
#include "egolib/Float.hpp"
#include "egolib/Debug.hpp"
#include "egolib/Math/Math.hpp"
#include "egolib/Math/TemplateUtilities.hpp"

namespace Ego {
namespace Math {

namespace Internal {

/**
 * @brief
 *  Derived from @a std::true_type if @a _VectorSpaceType::Dimensionality and @a ArgTypes
 *  fulfil the requirements for a constructor of a tuple, and derived from @a std::false_type
 *  otherwise.
 * @param _VectorSpaceType
 *  must fulfil the <em>vector space</em> concept
 * @param _ArgTypes
 *  @a ArgTypes must have <tt>_VectorSpaceType::Dimensionality-1</tt> elements which are convertible into values of type @a _VectorSpaceType::ScalarType
 * @author
 *  Michael Heilmann
 */
template <typename _VectorSpaceType, typename ... ArgTypes>
struct TupleConstructorEnable
    : public std::conditional<
      ((Ego::Core::EqualTo<sizeof...(ArgTypes), _VectorSpaceType::dimensionality() - 1>::value)
	   &&
	   (Ego::Core::AllTrue<std::is_convertible<ArgTypes,typename _VectorSpaceType::ScalarType>::value ...>::value)),
      std::true_type,
      std::false_type
      >::type
{};

} // namespace Internal

/**
 * @brief
 *	A tuple is the base class of vectors and points.
 * @tparam _UnderlayingType
 *	the underlaying type. Must fulfil the ordered integral domain concept.
 * @author
 *	Michael Heilmann
 */
template <typename _UnderlayingType>
struct Tuple {

public:

	/**
	 * @brief
	 *  @a MyType is the type of this template/template specialization.
	 */
	typedef Tuple<_UnderlayingType> MyType;

	/**
	 * @brief
	 *	@a VectorSpaceType is the type of the vector space.
	 */
	typedef _UnderlayingType VectorSpaceType;

	/**
	 * @brief
	 *  @a ScalarFieldType is the type of the underlaying scalar field.
	 */
	typedef typename _UnderlayingType::ScalarFieldType ScalarFieldType;

	/**
	 * @brief
	 *  @a ScalarType is the type of the underlaying scalars.
	 */
	typedef typename _UnderlayingType::ScalarFieldType::ScalarType ScalarType;

	/**
	 * @invariant
	 *  The dimensionality be a positive integral constant.
	 */
	static_assert(IsDimensionality<_UnderlayingType::dimensionality()>::value, "_UnderlayingType::Dimensionality must fulfil the dimensionality concept");

	/**
	 * @brief
	 *	The dimensionality of this tuple.
	 * @return
	 *	the dimensionality of this tuple
	 */
	constexpr static size_t dimensionality() {
		return _UnderlayingType::dimensionality();
	}

protected:

	/**
	 * @brief
	 *  The elements of this tuple.
	 */
	std::array<ScalarType, _UnderlayingType::dimensionality()> _elements;

	/**
	 * @brief
	 *	Construct this tuple with the specified element values.
	 * @param v, ... args
	 *	the element values
	 */
	template<typename ... ArgTypes, typename = typename std::enable_if<Internal::TupleConstructorEnable<_UnderlayingType, ArgTypes ...>::value>::type>
	Tuple(ScalarType v, ArgTypes&& ... args)
		: _elements{ v, args ... } {
		static_assert(_UnderlayingType::dimensionality() - 1 == sizeof ... (args), "wrong number of arguments");
	}

	/**
	 * @brief
	 *	Default-construct this tuple.
	 */
	Tuple() : _elements() {}

	/**
	 * @brief
	 *  Copy-construct this tuple with the values of another tuple.
	 * @param other
	 *  the other tuple
	 */
	Tuple(const MyType& other) : _elements(other._elements) {}

	/**
	 * @brief
	 *  Assign this tuple with the values of another tuple.
	 * @param other
	 *  the other tuple
	 */
	void assign(const MyType& other) {
		_elements = other._elements;
	}

	/**
	 * @{
	 * @brief
	 *	Get the tuple element at the specified index.
	 * @param index
	 *	the index
	 * @return
	 *	the tuple element at the specified index
	 * @pre
	 *	The index is within bounds.
	 */
	ScalarType& at(size_t const& index) {
	#ifdef _DEBUG
		EGOBOO_ASSERT(index < dimensionality());
	#endif
		return _elements[index];
	}

	const ScalarType& at(size_t const& index) const {
	#ifdef _DEBUG
		EGOBOO_ASSERT(index < dimensionality());
	#endif
		return _elements[index];
	}

	/**@}*/
    
};

} // namespace Math
} // namespace Ego
