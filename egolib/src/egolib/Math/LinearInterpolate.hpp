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

/// @file egolib/Math/LinearInterpolate.hpp
/// @brief Linear interpolation.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/TemplateUtilities.hpp"

namespace Ego {
namespace Math {

/**
 * @brief Linear interpolation functor.
 */
template <typename Type, typename _EnabledType = void>
struct Lerp;

template <typename ResultType, typename OperandType, typename ParameterType>
ResultType lerp(const OperandType& x, const OperandType& y, const ParameterType& t);

} // namespace Math
} // namespace Ego
