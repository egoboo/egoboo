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

/// @file egolib/Math/Functors/Interpolate_Linear_Point.hpp
/// @brief Linear interpolation for point types.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Point.hpp"

namespace id {

/**
 * @ingroup math
 * @brief
 *  Linear interpolation function for point types.
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
template <typename VectorSpace>
struct interpolation_functor<Ego::Math::Point<VectorSpace>, interpolation_method::LINEAR, void>
{
    using point = Ego::Math::Point<VectorSpace>;
    using vector = Ego::Math::Vector<typename VectorSpace::ScalarField, VectorSpace::dimensionality()>;
    using scalar = typename VectorSpace::ScalarType;

    point operator()(const point& x, const point& y, scalar t) const
    {
        return (*this)(x, y, mu<scalar>(t));
    }

    point operator()(const point& x, const point& y, const mu<scalar>& mu) const
    {
        id::interpolation_functor<vector, interpolation_method::LINEAR> f;
        return point::toPoint(f(point::toVector(x), point::toVector(y), mu));
    }

}; // struct interpolate_functor

} // namespace id
