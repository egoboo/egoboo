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

/// @file egolib/Math/Functors/Closure_Sphere_Sphere.hpp
/// @brief Enclose a sphere in a sphere.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Closure.hpp"

namespace Ego {
namespace Math {

/// Enclose a sphere into a sphere.
/// The axis aligned box closure \f$C(A)\f$ of an axis aligned box \f$A\f$ is \f$A\f$ itself i.e. \f$C(A) = A\f$.
template <typename _EuclideanSpaceType>
struct Closure<Sphere<_EuclideanSpaceType>, Sphere<_EuclideanSpaceType>> {
public:
    using EuclideanSpaceType = _EuclideanSpaceType;
    using SourceType = Sphere<EuclideanSpaceType>;
    using TargetType = Sphere<EuclideanSpaceType>;
public:
    inline TargetType operator()(const SourceType& source) const {
        return source;
    }
}; // struct Closure

} // namespace Math
} // namespace Ego
