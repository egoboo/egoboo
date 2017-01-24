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

/// @file egolib/Math/Functors/Interpolate_Linear_ColourRGB.hpp
/// @brief Linear interpolation for RGB colour types with real type components
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Interpolate.hpp"
#include "egolib/Math/Functors/Interpolate_Linear_Real.hpp"

namespace Ego {
namespace Math {

template <typename ColourSpaceTypeArg>
struct Interpolate<Id::Colour<ColourSpaceTypeArg>, InterpolationMethod::Linear, std::enable_if_t<std::is_same<Id::RGBf, ColourSpaceTypeArg>::value>> {
    using A = Id::Colour<ColourSpaceTypeArg>;
    using ColourSpaceType = ColourSpaceTypeArg;
    /// @ingroup math
    /// @brief Generic linear interpolation: specialization for Id::Colour<Id::RGBf>.
    /// @param x, y the values to interpolate between
    /// @param t the parameter. Must be within the bounds of @a 0 (inclusive) and @a 1 (inclusive).
    /// @return the interpolated value
    A operator()(const A& x, const A& y, float t) const {
        static const Interpolate<float,InterpolationMethod::Linear> f{};
        /// @todo Use a cookie constructor: The values are within bounds.
        return A(constrain(f(x.getRed(), y.getRed(), t), ColourSpaceType::min(), ColourSpaceType::max()),
                 constrain(f(x.getGreen(), y.getGreen(), t), ColourSpaceType::min(), ColourSpaceType::max()),
                 constrain(f(x.getBlue(), y.getBlue(), t), ColourSpaceType::min(), ColourSpaceType::max()));
    }

}; // struct Interpolate

} // namespace Math
} // namespace Ego
