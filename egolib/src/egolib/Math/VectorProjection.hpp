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

/// @file   egolib/Math/VectorProjection.hpp
/// @brief  Projection of a vector.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Vector.hpp"

namespace Ego {
namespace Math {

/**
 * Compute the vector projection of a vector \f$\vec{v}\f$ onto a vector \f$\vec{w}\f$.
 * @param v the vector \f$\vec{v}\f$
 * @param w the vector \f$\vec{w}\f$
 * @return the projection of this vector \f$\vec{v}\f$ onto a vector \f$\vec{w}\f$
 * @remark The vector projection is defined as
 * \f{align*}
 * proj(\vec{v},\vec{w}) = \frac{\vec{v} \cdot \vec{w}}{|\vec{w}|^2} \vec{w}
 * \f}
 * which is the definition this implementation is using.
 * @throw Id::RuntimeErrorException if the vector \f$\vec{w}\f$ is the zero vector
 */
template <typename _VectorType>
inline _VectorType Projection(const _VectorType& v, const _VectorType& w) {
    _VectorType l = w.length_2();
    if (l == _VectorType::ScalarType()) {
        Id::RuntimeErrorException(__FILE__, __LINE__, "vector w must not be the zero vector");
    }
    return w * (w.dot(v) / l);
}

} // namespace Math
} // namespace Ego


