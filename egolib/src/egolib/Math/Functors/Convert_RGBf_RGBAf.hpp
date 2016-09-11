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

/// @file egolib/Math/Functor/Convert_RGBf_RGBAf.hpp
/// @brief Color conversion functor: Lossy conversion from RGBAf color space to RGBf color space.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Convert.hpp"
#include "egolib/Math/ColourRGB.hpp"
#include "egolib/Math/ColourRGBA.hpp"

namespace Ego {
namespace Math {

/// Convert colours from RGBAf to RGBf.
/// \f$(r,g,b,a)\f$ is mapped to \f$(r,g,b)\f$.
template <>
struct Convert<Colour<RGBf>, Colour<RGBAf>> {
    Colour<RGBf> operator()(const Colour<RGBAf>& source) const {
        return Colour<RGBf>(source.getRed(), source.getGreen(), source.getBlue());
    }
};

} // namepace Math
} // namespace Ego
