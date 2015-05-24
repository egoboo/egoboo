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

/// @file   egolib/Math/Scalar.hpp
/// @brief  Functionality related to the concept of scalar.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/TemplateUtilities.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  A struct derived from @a std::true_type if @a _ScalarType fulfils the
 *  properties of the <em>scalar concept</em> and derived from
 *  @a std::false_type otherwise.
 * @author
 *  Michael Heilmann
 */
template <typename _ScalarType>
struct IsScalar
    : public std::conditional<
        (std::is_floating_point<_ScalarType>::value),
        std::true_type,
        std::false_type
      >::type {};

} // namespace Math
} // namespace Ego
