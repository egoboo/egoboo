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

#pragma once

#include "egolib/Math/VectorSpace.hpp"
#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/AxisAlignedCube.hpp"
#include "egolib/Math/Sphere.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  Functor computing the closure of a volume. The closure funcion $C[B,A]$ maps a volumes of type
 *  \f$A\f$ to the set of volumes of type \f$B\f$. Considering volumes as sets of points for which
 *  set equality relation \f$=\f$ and the subset relation are defined, then the following properties
 *  hold: 
 *  - \f$x \subseteq C[B,A](x)\f$
 *  - \f$x \subseteq y \Rightarrow C[B,A](x) \subseteq C[B,A](y)\f$
 *  - \f$C[B,A](x) = C[B,B](C[B,A](x))\f$ 
 *  where \f$a\f$ and \f$b\f$ are volumes of type \f$a\f$ and \f$b\f$.
 *  Assuming that \f$C[B,B]\f$ is defined for all supported types and
 *  is simply the identity function \f$C[B,B](x) = Id[B](x)\f$
 * @tparam _SourceType
 *  the type of the enclosed values.
 *  A member typedef @a SourceType is provided.
 *  the source volume
 * @param _TargetType
 *  the type of the enclosing values.
 *  A member typedef @a TargetType is provided.
 * @remark
 *  If a specialization exists, then the specialization provides defines a `operator()` that
 *  - accepts a single argument value of type @a _SourceType
 *  - returns a single return value of type @a _TargetType
 *  such that
 *  - the returned value represents the value of type @a _TargetType enclosing the accepted value
 *  of type @a _SourceType.
 */
template <typename _TargetType, typename _SourceType>
struct Closure;

} // namespace Math
} // namespace Ego
