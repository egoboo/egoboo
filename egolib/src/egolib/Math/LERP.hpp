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

/// @file   egolib/Math/LERP.hpp
/// @brief  Linear interpolation.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

struct fvec2_t;
struct fvec3_t;

namespace Ego
{
namespace Math
{

class Colour3f;
class Colour4f;

/**
 * @ingroup math
 * @brief
 *  Generic linear interpolation function.
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
 * @throws std::domain_error
 *  if @a t is smaller than @a 0 and greater than @a 1
 */
template <typename Type>
Type lerp(const Type& x, const Type& y, const float t);

/**
 * @ingroup math
 * @brief
 *  Generic linear interpolation: Specialization for floats.
 * @param x, y
 *  the values to interpolate between
 * @param t
 *  the parameter.
 *  Must be within the bounds of @a 0 (inclusive) and @a 1 (inclusive).
 * @return
 *  the interpolated value
 * @remark
 *  This implementation is precise as
 *  @code
 *  v = (1.0f - t) * x + t * y;
 *  @endcode
 *  which guarantees v = v1 when t = 1.
 *  The alternative implementation is not precise as
 *  @code
 *  v = x + t*(y - x);
 *  @endcode
 *  which does not guarantee v = v1 when t = 1
 *  due to floating-point arithmetic error.
 */
template <>
float lerp<float>(const float& x, const float& y, float t);

/**
 * @ingroup math
 * @brief
 *  Generic linear interpolation: specialization for Ego::Math::Vector2.
 * @param x, y
 *  the values to interpolate between
 * @param t
 *  the parameter.
 *  Must be within the bounds of @a 0 (inclusive) and @a 1 (inclusive).
 * @return
 *  the interpolated value
 */
template <>
fvec2_t lerp<fvec2_t>(const fvec2_t& x, const fvec2_t& y, float t);

/**
 * @ingroup math
 * @brief
 *  Generic linear interpolation: specialization for Ego::Math::Vector2.
 * @param x, y
 *  the values to interpolate between
 * @param t
 *  the parameter.
 *  Must be within the bounds of @a 0 (inclusive) and @a 1 (inclusive).
 * @return
 *  the interpolated value
 */
template <>
fvec3_t lerp<fvec3_t>(const fvec3_t& x, const fvec3_t& y, float t);

/**
 * @ingroup math
 * @brief
 *  Generic linear interpolation: specialization for Ego::Math::Vector2.
 * @param x, y
 *  the values to interpolate between
 * @param t
 *  the parameter.
 *  Must be within the bounds of @a 0 (inclusive) and @a 1 (inclusive).
 * @return
 *  the interpolated value
 */
template <>
Colour3f lerp<Colour3f>(const Colour3f& x, const Colour3f& y, float t);

/**
 * @ingroup math
 * @brief
 *  Generic linear interpolation: specialization for Ego::Math::Vector2.
 * @param x, y
 *  the values to interpolate between
 * @param t
 *  the parameter.
 *  Must be within the bounds of @a 0 (inclusive) and @a 1 (inclusive).
 * @return
 *  the interpolated value
 */
template <>
Colour4f lerp<Colour4f>(const Colour4f& x, const Colour4f& y, float t);

} // namespace Math
} // namespace Ego
