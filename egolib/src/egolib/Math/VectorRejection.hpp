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

/// @file   egolib/Math/VectorRejection.hpp
/// @brief  Rejection of a vector.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/VectorRejection.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Compute the rejection of a vector \f$\vec{v}\f$ onto a vector \f$\vec{w}\f$.
 * @param v the vector \$\vec{v}\f$
 * @param w the vector \$\vec{w}\f$
 * @return the rejection of the vector \$v\f$ onto a vector \f$w\f$
 * @remark The vector rejeciton of a vector \f$v\f$ onto a vector \f$w\f$
 * is defined as
 * \f{align*}
 * rej(\vec{v},\vec{w}) = \vec{v} - proj(\vec{v},\vec{w}) 
 * \f}
 * where \f$proj(\vec{v},\vec{w})\f$ is the vector projection of the vector \f$\vec{v}\f$ onto the vector \f$\vec{w}\f$.
 * @throw Id::RuntimeErrorException if the vector \$\vec{w}\f$ is the zero vector
 */
template <typename _VectorType>
inline _VectorType Rejection(const _VectorType& v, const _VectorType& w) {
    return v - Projection(v,w);
}

} // namespace Math
} // namespace Ego
