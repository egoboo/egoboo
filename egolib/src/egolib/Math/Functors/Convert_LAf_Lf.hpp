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

/// @file egolib/Math/Functor/Convert_LAf_Lf.hpp
/// @brief Color conversion functor: Conversion from Lf color space to LAf color space.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Convert.hpp"
#include "egolib/Math/ColourL.hpp"
#include "egolib/Math/ColourLa.hpp"

namespace Ego {
namespace Math {

/// Convert a colour from Lf to LAf.
/// \f$(l)\f$ is mapped to \f$(l,a)\f$ where \f$a\f$ is the specified alpha value.
template <>
struct Convert<Colour<LAf>, Colour<Lf>> {
protected:
    float alpha;
public:
    Convert(float alpha = LAf::max()) : alpha(alpha) {}
    Colour<LAf> operator()(const Colour<Lf>& source) const {
        return Colour<LAf>(source.getLuminance(), alpha);
    }
};

} // namespace Math
} // namespace Ego
