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

namespace Ego {
namespace Math {

namespace Internal {

/**
 * @brief
 *  Derived from @a std::true_type if @a _VectorSpaceType::Dimensionality and @a ArgTypes fulfil the requirements for a constructor of a tuple,
 *	and derived from @a std::false_type otherwise.
 * @param _VectorSpaceType
 *  must fulfil the <em>vector space</em> concept
 * @param _ArgTypes
 *  @a ArgTypes must have <tt>_VectorSpaceType::Dimensionality-1</tt> elements which are convertible into values of type @a _VectorSpaceType::ScalarType
 * @author
 *  Michael Heilmann
 * @todo
 *  Fast-fail if the parameters are not convertible into @a _VectorSpaceType::ScalarType.
 */
template <typename _VectorSpaceType, typename ... ArgTypes>
struct TupleConstructorEnable
    : public std::conditional<
      (Ego::Core::EqualTo<sizeof...(ArgTypes), _VectorSpaceType::Dimensionality::value - 1>::value),
      std::true_type,
      std::false_type
      >::type
{};

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

/**
 * @brief
 *	A tuple is the base class of vectors and points.
 * @author
 *	Michael Heilmann
 */
template <typename _VectorSpaceType>
struct Tuple {
    
public:
    
    /**
     * @brief
     *  @a MyType is the type of this template/template specialization.
     */
    typedef Tuple<_VectorSpaceType> MyType;

	/**
	 * @brief
	 *	@a VectorSpaceType is the type of the vector space.
	 */
	typedef _VectorSpaceType VectorSpaceType;

	/**
	 * @brief
	 *  @a ScalarFieldType is the type of the underlaying scalar field.
	 */
	typedef typename _VectorSpaceType::ScalarFieldType ScalarFieldType;

	/**
	 * @brief
	 *  @a ScalarType is the type of the underlaying scalars.
	 */
	typedef typename _VectorSpaceType::ScalarFieldType::ScalarType ScalarType;


    

    
    /**
     * @invariant
     *  The dimensionality be a positive integral constant.
     */
    static_assert(IsDimensionality<_VectorSpaceType::Dimensionality::value>::value, "VectorSpaceType::Dimensionality must fulfil the dimensionality concept");

	/**
	 * @brief
	 *	The dimensionality of this tuple.
	 * @return
	 *	the dimensionality of this tuple
	 */
	static size_t dimensionality() {
		return VectorSpaceType::Dimensionality::value;
	}
    
protected:

    /**
     * @brief
     *  The elements of this tuple.
     */
	ScalarType _elements[VectorSpaceType::Dimensionality::value];

	/**
	 * @brief
	 *	Construct this tuple with the specified element values.
	 * @param v, ... args
	 *	the element values
	 */
	template<typename ... ArgTypes, typename = typename std::enable_if<Internal::TupleConstructorEnable<_VectorSpaceType, ArgTypes ...>::value>::type>
	Tuple(ScalarType v, ArgTypes&& ... args) {
		static_assert(_VectorSpaceType::Dimensionality::value - 1 == sizeof ... (args), "wrong number of arguments");
		Internal::unpack<float, _VectorSpaceType::Dimensionality::value>(_elements, std::forward<ScalarType>(v), args ...);
	}

	/**
	 * @brief
	 *	Default-construct this tuple.
	 */
	Tuple() :
		_elements() {}

	/**
	 * @brief
	 *  Copy-construct this tuple with the values of another tuple.
	 * @param other
	 *  the other tuple
	 */
	Tuple(const MyType& other) {
		for (size_t i = 0; i < dimensionality(); ++i) {
			_elements[i] = other._elements[i];
		}
	}

	/**
	 * @brief
	 *  Assign this tuple with the values of another tuple.
	 * @param other
	 *  the other tuple
	 */
	void assign(const MyType& other) {
		for (size_t i = 0; i < dimensionality(); ++i) {
			_elements[i] = other._elements[i];
		}
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
