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

#pragma once

#include "egolib/Math/VectorSpace.hpp"
#include "egolib/Math/AABB.hpp"
#include "egolib/Math/Sphere.h"

namespace Ego {
namespace Math {

/**
 * @brief
 *  Compute the smallest volume enclosing a given volume.
 * @param source
 *  the source volume
 * @param target
 *  the target volume
 */
template <typename SourceType, typename TargetType>
TargetType convexHull(const SourceType& source);

template <>
inline Sphere<VectorSpace<Field<float>, 3>> convexHull(const AABB<VectorSpace<Field<float>, 3>>& source) {
    const auto center = source.getCenter();
    const auto radius = (source.getMin() - center).length();
    return Sphere<VectorSpace<Field<float>, 3>>(center, radius);
}


} // namespace Math
} // namespace Ego
