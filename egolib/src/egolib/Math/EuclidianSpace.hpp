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

/// @file   egolib/Math/EuclidianSpace.hpp
/// @brief  The \f$n\f$-dimensional Euclidian space (aka \f$n\f$-dimensional Cartesian space).
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Vector.hpp"
#include "egolib/Math/Point.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  An \f$n\f$-dimensional Euclidian space (sometimes called \f$n\f$-dimensional Cartesian space).
 * @tparam _VectorSpaceType 
 *  the underlying \f$n\f$-dimensional vector space type.
 *  Must fulfil the <em>VectorSpace</tt> concept.
 */
template <typename _VectorSpaceType,
          typename _Enabled = void>
struct EuclidianSpace;

template <typename _VectorSpaceType>
struct EuclidianSpace<_VectorSpaceType, std::enable_if_t<true>> {
public:
    /**
     * @brief
     *  The type of the vector space.
     */
    typedef _VectorSpaceType VectorSpaceType;

    /**
     * @brief
     *  The type of this template/template specialization.
     */
    typedef EuclidianSpace<VectorSpaceType> MyType;

    /**
     * @brief
     *  The type of the scalar field.
     */
    typedef typename VectorSpaceType::ScalarFieldType ScalarFieldType;

    /**
     * @brief
     *  Get the dimensionality.
     * @return
     *  the dimensionality
     */
    static constexpr size_t dimensionality() {
        return VectorSpaceType::dimensionality();
    }

    /**
     * @brief
     *  The type of a scalar.
     */
    typedef typename VectorSpaceType::ScalarType ScalarType;

    /**
     * @brief
     *  The type of a vector.
     */
    typedef typename VectorSpaceType::VectorType VectorType;

    /**
     * @brief
     *  The type of a point.
     */
    typedef Point<VectorSpaceType> PointType;

}; // struct EuclidianSpace

} // namespace Math
} // namespace Ego
