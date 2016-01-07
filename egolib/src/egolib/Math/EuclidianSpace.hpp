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

namespace Ego {
namespace Math {

/**
* @brief An \f$n\f$-dimensional Euclidian space (sometimes called \f$n\f$-dimensional Cartesian space).
* @tparam the underlying \f$n\f$-dimensional vector space
*/
template <typename _VectorSpaceType>
struct EuclidianSpace {
public:
    /** @brief The type of the vector space. */
    typedef _VectorSpaceType VectorSpaceType;

    /** @brief The type of the scalar field. */
    typedef typename VectorSpaceType::ScalarFieldType ScalarFieldType;

public:
    /** @brief The type of a vector. */
    typedef Vector<_VectorSpaceType> VectorType;

    /** @brief The type of a scalar. */
    typedef typename VectorSpaceType::ScalarType ScalarType;


public:
    /**
     * @brief Get the dimensionality.
     * @return the dimensionality
     */
    constexpr static size_t dimensionality() {
        return VectorSpaceType::dimensionality();
    }

}; // struct EuclidianSpace

} // namespace Math
} // namespace Ego
