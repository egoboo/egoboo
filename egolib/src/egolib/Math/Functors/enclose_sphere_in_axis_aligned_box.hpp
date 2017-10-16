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

/// @file egolib/Math/Functors/Closure_AxisAlignedBox_Sphere.hpp
/// @brief Enclose spheres in axis aligned boxes.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/Sphere.hpp"

namespace id {

/// @brief Specialization of id::enclose_functor.
/// Encloses a sphere in an axis aligned box.
/// @detail The axis aligned box \f$b\f$ enclosing a sphere \f$a\f$ with
/// center \f$c\f$ and radius \f$r\f$ has the minimal point \f$min = c -
/// \vec{1} \cdot r\f$ and the maximal point \f$c + \vec{1} \cdot  r\f$.
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct enclose_functor<Ego::Math::AxisAlignedBox<E>, 
	                   Ego::Math::Sphere<E>>
{
    auto operator()(const Ego::Math::Sphere<E>& source) const
	{
		const auto center = source.getCenter();
		const auto radius = source.getRadius();
        const auto extend = one<VectorType>() * radius;
        return Ego::Math::AxisAlignedBox<E>(center - extend, center + extend);
    }
private:
	using VectorType = typename E::VectorType;
}; // struct enclose_functor

} // namespace id
