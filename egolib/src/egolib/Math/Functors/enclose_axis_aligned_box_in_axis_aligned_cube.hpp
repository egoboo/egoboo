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

/// @file egolib/Math/Functors/Closure_AxisAlignedBox_AxisAlignedBox.hpp
/// @brief Enclose an axis aligned cubes in an axis aligned boxes.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace idlib {

/// @brief Specialization of idlib::enclose_functor.
/// Encloses an axis aligned box in an axis aligned cube.
/// @detail Given an axis aligned box \f$a\f$ with edge lengths \f$l_0, \ldots, l_{n-1}\f$ and
/// a center \f$c\f$, an axis aligned cube \f$b\f$ enclosing the axis aligned box can be
/// computed as follows:
/// - The center \f$c'\f$ of the cube is the center of the box i.e. \f$c' := c\f$.
/// - The edge length of the cube \f$l'\f$ is the maximum edge length of the box i.e. \f$l' := \max_{i=0}^{n-1} l_i\f$
template <typename P>
struct enclose_functor<axis_aligned_cube<P>, axis_aligned_box<P>>
{
    auto operator()(const axis_aligned_box<P>& source) const
	{
		// Get the edge lengths of the box.
		auto edge_lengths = source.get_size();
		// Get the center of the box.
		auto center = source.get_center();
		// Get the maximal edge length of the box.
		auto maximal_edge_length = max_element(edge_lengths);
		// Create the cube.
        return axis_aligned_cube<P>(center, maximal_edge_length);
    }
}; // struct enclose_functor

} // namespace idlib
