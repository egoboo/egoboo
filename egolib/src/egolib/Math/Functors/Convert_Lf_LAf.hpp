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

/// @file egolib/Math/Functor/Convert_Lf_LAf.hpp
/// @brief Color conversion functor: Lossy conversion from LAf color space to Lf color space.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Convert.hpp"
#include "egolib/Math/ColourL.hpp"
#include "egolib/Math/ColourLA.hpp"

namespace Ego {
namespace Math {

/// Convert colours from LAf to Lf.
/// \f$(l,a)\f$ is mapped to \f$(l)\f$.
template <>
struct Convert<Colour<Lf>, Colour<LAf>> {
    Colour<Lf> operator()(const Colour<LAf>& source) const {
        return Colour<Lf>(source.getLuminance());
    }
};

} // namespace Math
} // namespace Ego
