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

/// @file egolib/Math/Functors/Closure_AxisAlignedCube_AxisAlignedCube.hpp
/// @brief Enclose a axis aligned cube in an axis aligned cube.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Closure.hpp"

namespace Ego {
namespace Math {

/// Enclose an axis aligned cube into an axis aligned cube.
/// The axis aligned cube closure \f$C(A)\f$ of an axis aligned cube \f$A\f$ is \f$A\f$ itself i.e. \f$C(A) = A\f$.
template <typename _EuclideanSpaceType>
struct Closure<AxisAlignedCube<_EuclideanSpaceType>, AxisAlignedCube<_EuclideanSpaceType>> {
public:
    using EuclideanSpaceType = _EuclideanSpaceType;
    using SourceType = AxisAlignedCube<EuclideanSpaceType>;
    using TargetType = AxisAlignedCube<EuclideanSpaceType>;
public:
    inline TargetType operator()(const SourceType& source) const {
        return source;
    }
}; // struct Closure

} // namespace Math
} // namespace Ego
