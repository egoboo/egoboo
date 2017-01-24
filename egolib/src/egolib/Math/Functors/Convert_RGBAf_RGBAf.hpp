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

/// @file egolib/Math/Functor/Convert_RGBAf_RGBAf.hpp
/// @brief Identity color conversion functor.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Convert.hpp"

namespace Ego {
namespace Math {

/// Convert a colour from RGBAf to RGBAf.
/// This is an identity conversion.
template <>
struct Convert<Id::Colour<Id::RGBAf>, Id::Colour<Id::RGBAf>> {
    Id::Colour<Id::RGBAf> operator()(const Id::Colour<Id::RGBAf>& source) const {
        return source;
    }
};

} // namespace Math
} // namespace Ego
