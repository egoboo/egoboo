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
/// @brief  Tuples.
/// @author Michael Heilmann

#pragma once



#include "egolib/Math/Dimensionality.hpp"
#include "egolib/Math/TemplateUtilities.hpp"



namespace Ego {
namespace Math {

namespace Internal {

/**
 * @brief
 *  Derived from @a std::true_type if @a _Dimensionality and @a ArgTypes fulfil the requirements
 *  for a constructor of a tuple, and derived from @a std::false_type otherwise.
 * @param _ElementType
 *  the type of the elements of the tuple
 * @param _Dimensionality
 *  the dimensionality of the tuple
 * @param _ArgTypes
 *  @a ArgTypes must have <tt>_Dimensionality-1</tt> elements which are convertible into values of type @a _ElementType
 */
template <typename _Type, size_t _Dimensionality, typename ... ArgTypes>
struct TupleConstructorEnable
    : public std::conditional<
      ((Ego::Core::EqualTo<sizeof...(ArgTypes), _Dimensionality - 1>::value)
	   &&
	   (Ego::Core::AllTrue<std::is_convertible<ArgTypes,typename _Type>::value ...>::value)),
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
template <typename _ElementType, size_t _Dimensionality>
struct Tuple {
public:
    /** @brief The type of the elements of this tuple type. */
    typedef _ElementType ElementType;

    /**
     * @brief The dimensionality of this tuple.
     * @return the dimensionality of this tuple
     */
    constexpr static size_t dimensionality() {
        return _Dimensionality;
    }

    /** @invariant The dimensionality be a positive integral constant. */
    static_assert(IsDimensionality<_Dimensionality>::value, "_Dimensionality must fulfil the dimensionality concept");


	/** @brief @a MyType is the type of this template/template specialization. */
	typedef Tuple<ElementType, _Dimensionality> MyType;


protected:
	/**
	 * @brief
	 *  The elements of this tuple.
	 */
	std::array<ElementType, _Dimensionality> _elements;

	/**
	 * @brief
	 *	Construct this tuple with the specified element values.
	 * @param v, ... args
	 *	the element values
	 */
	template<typename ... ArgTypes, typename = std::enable_if_t<Internal::TupleConstructorEnable<_ElementType, _Dimensionality, ArgTypes ...>::value>>
	Tuple(ElementType v, ArgTypes&& ... args)
		: _elements{ v, args ... } {
		static_assert(dimensionality() - 1 == sizeof ... (args), "wrong number of arguments");
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
	ElementType& at(size_t const& index) {
	#ifdef _DEBUG
		ID_ASSERT(index < dimensionality());
	#endif
		return _elements[index];
	}

	const ElementType& at(size_t const& index) const {
	#ifdef _DEBUG
		ID_ASSERT(index < dimensionality());
	#endif
		return _elements[index];
	}

};

} // namespace Math
} // namespace Ego
