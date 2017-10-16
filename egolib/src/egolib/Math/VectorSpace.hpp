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
	  id::is_dimensionality_v<_Dimensionality>,
	  std::true_type,
	  std::false_type
	  >::type
{};

} // namespace Internal

/**
 * @brief
 *	An \f$n\f$-dimensional vector space.
 * @tparam _ScalarFieldType
 *  the underlying scalar field type.
 *  Must fulfil the <em>ScalarField</em> concept.
 * @tparam _Dimensionality
 *  the dimensionality of the vector space.
 *  Must fulfil the <em>Dimensionality</em> concept.
 */
template <typename _ScalarFieldType, size_t _Dimensionality, 
		  typename _Enabled = void>
struct VectorSpace;

template <typename _ScalarFieldType, size_t _Dimensionality>
struct VectorSpace<_ScalarFieldType, _Dimensionality, 
	               std::enable_if_t<Internal::VectorSpaceEnable<_ScalarFieldType, _Dimensionality>::value>> {
public:
    /**
     * @brief
     *  The scalar field type.
     */
    using ScalarFieldType = _ScalarFieldType;

    /**
     * @brief
     *  The scalar type.
     */
    using ScalarType = typename ScalarFieldType::ScalarType;

    /**
     * @brief
     *  The type of this template/template specialization.
     */
    using MyType = VectorSpace<ScalarFieldType, _Dimensionality>;

    /**
     * @brief
     *  The dimensionality of the vector space.
     * @return
     *  the dimensionality of the vector space
     */
    constexpr static size_t dimensionality() {
        return _Dimensionality;
    }

    /**
     * @brief
     *  The vector type.
     */
    using VectorType = id::vector<typename ScalarFieldType::ScalarType, MyType::dimensionality()>;

}; // struct VectorSpace

} // namespace Math
} // namespace Ego
