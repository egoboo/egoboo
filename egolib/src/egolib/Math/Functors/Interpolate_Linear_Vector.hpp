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

#include "egolib/Math/Functors/Interpolate.hpp"
#include "egolib/Math/Vector.hpp"
#include "idlib/math/one_zero.hpp"

namespace Ego {
namespace Math {

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
 * @throws Id::OutOfBoundsException
 *  if @a t is smaller than @a 0 and greater than @a 1
 */
template <typename _ScalarFieldType, size_t _Dimensionality>
struct Interpolate<Vector<_ScalarFieldType, _Dimensionality>, InterpolationMethod::Linear, void> {
    using A = Vector<_ScalarFieldType, _Dimensionality>;
    using T = typename _ScalarFieldType::ScalarType;
    A operator()(A x, A y, T t) const {
        if (t < id::zero<T>() || t > id::one<T>()) {
            Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
            e << "parameter t = " << t << " not within the interval of [0,1]" << Log::EndOfEntry;
            Log::get() << e;
            throw Id::OutOfBoundsException(__FILE__, __LINE__, e.getText());
        }
        return x * (id::one<T>() - t) + y * t;
    }
}; // struct Interpolate

} // namespace Math
} // namespace Ego
