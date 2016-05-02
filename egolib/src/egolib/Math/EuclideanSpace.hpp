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

/// @file   egolib/Math/EuclideanSpace.hpp
/// @brief  The \f$n\f$-dimensional Euclidean space (aka \f$n\f$-dimensional Cartesian space).
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Point.hpp"
#include "egolib/Math/Vector.hpp"
#include "egolib/Math/OrderedField.hpp"
#include "egolib/Math/VectorSpace.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  An \f$n\f$-dimensional Euclidean space (sometimes called \f$n\f$-dimensional Cartesian space).
 * @tparam _VectorSpaceType 
 *  the underlying \f$n\f$-dimensional vector space type.
 *  Must fulfil the <em>VectorSpace</em> concept.
 */
template <typename _VectorSpaceType,
          typename _Enabled = void>
struct EuclideanSpace;

template <typename _VectorSpaceType>
struct EuclideanSpace<_VectorSpaceType, std::enable_if_t<true>> {
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
    typedef EuclideanSpace<VectorSpaceType> MyType;

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

private:
    /**
     * @brief Invoke the constructor of type <tt>TargetType</tt> with
     * passing the arguments <tt>source[0]</tt>,<tt>source[1]</tt>, ...,
     * <tt>source[n]</tt>. 
     * @todo Can we re-use this in other places?
     */
    template <typename TargetType, typename SourceType, std::size_t... Index>
    static decltype(auto) invoke(const SourceType& source, std::index_sequence<Index ...>) {
        return TargetType(source[Index]...);
    }

public:
    static decltype(auto) toVector(const PointType& source) {
        return invoke<VectorType>(source, std::make_index_sequence<MyType::dimensionality()>{});
    }
    static decltype(auto) toPoint(const VectorType& source) {
        return invoke<PointType>(source, std::make_index_sequence<MyType::dimensionality()>{});
    }

}; // struct EuclideanSpace

} // namespace Math
} // namespace Ego
