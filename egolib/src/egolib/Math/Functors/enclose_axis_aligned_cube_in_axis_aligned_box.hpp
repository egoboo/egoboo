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

#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/AxisAlignedCube.hpp"

namespace id {

/// @brief Specialization of id::enclose_functor.
/// Encloses an axis aligned cube in an axis aligned box.
/// @detail Let \$a\f$ be an axis aligned cube and \f$min\f$ its minimal point and \f$max\f$ its maximal point.
/// The axis aligned box \f$b\f$ enclosing \f$a\f$ has the same minimal and maximal and maximal point.
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct enclose_functor<Ego::Math::AxisAlignedBox<E>,
	                   Ego::Math::AxisAlignedCube<E>>
{
    auto operator()(const Ego::Math::AxisAlignedCube<E>& source) const
	{ return Ego::Math::AxisAlignedBox<E>(source.getMin(), source.getMax()); }
}; // struct enclose_functor

} // namespace id
