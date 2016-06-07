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

/// @file egolib/Math/Closure_AxisAlignedCube_AxisAlignedCube.hpp
/// @brief Enclose a axis aligned cube in an axis aligned cube.
/// @author Michael Heilmann

#pragma once

namespace Ego {
namespace Math {

/// Enclose an axis aligned cube in an axis aligned box.
/// Let \f$min\f$ be the minimal point and \f$max\f$ be the maximal point of the axis aligned cube.
/// The axis aligned box with the same minimal and maximal point is the smallest axis aligned box enclosing that axis aligned cube.
template <typename _EuclideanSpaceType>
struct ConvexHull<AxisAlignedBox<_EuclideanSpaceType>, AxisAlignedCube<_EuclideanSpaceType>> {
public:
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef AxisAlignedCube<EuclideanSpaceType> SourceType;
    typedef AxisAlignedBox<EuclideanSpaceType> TargetType;
public:
    inline TargetType operator()(const SourceType& source) const {
        return TargetType(source.getMin(), source.getMax());
    }
}; // struct Closure

} // namespace Math
} // namespace Ego