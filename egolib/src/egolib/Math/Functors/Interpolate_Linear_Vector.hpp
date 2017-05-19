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

/// @file egolib/Math/Functors/Interpolate_Linear_Vector.hpp
/// @brief Linear interpolation for vector types.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Vector.hpp"

namespace id {

/**
 * @ingroup math
 * @brief
 *  Linear interpolation function for vector types.
 * @param x, y
 *  the values to interpolate between
 * @param t
 *  the parameter.
 *  Must be within the bounds of @a 0 (inclusive) and @a 1 (inclusive).
 * @return
 *  the interpolated value
 * @remark
 *  Template specializations of this template do in general not perform
 *  linear extrapolation, that is, @a t has to stay within the bound of
 *  @a 0 (inclusive) and @a 1 (inclusive). It is required that a
 *  specialization raises an std::domain_error if an argument
 *  value for @a t is outside of those bounds.
 * @throws id::out_of_bounds_error
 *  if @a t is smaller than @a 0 and greater than @a 1
 */
template <typename ScalarField, size_t Dimensionality>
struct interpolation_functor<Ego::Math::Vector<ScalarField, Dimensionality>, interpolation_method::LINEAR, void>
{
    using scalar = typename ScalarField::ScalarType;
    using vector = Ego::Math::Vector<ScalarField, Dimensionality>;

    vector operator()(const vector& x, const vector& y, scalar t) const
    {
        return (*this)(x, y, mu<scalar>(t));
    }

    vector operator()(const vector& x, const vector& y, const mu<scalar>& mu) const
    {
        return x * mu.getOneMinusValue() + y * mu.getValue();

    }

}; // struct interpolate_functor

} // namespace id
