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

/// @file egolib/Math/Translate_Point.hpp
/// @brief Translation of points.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Point.hpp"

namespace Ego {
namespace Math {

#if 0
template <typename _VectorSpaceType>
struct Translate<Point<_VectorSpaceType>> {
    typedef Point<_VectorSpaceType> X;
    typedef typename _VectorSpaceType::VectorType T;
    X operator()(const X& x, const T& t) const {
        return X(x.getA() + t, x.getB() + t);
    }
};
#endif

#if 0
template <typename _VectorSpaceType>
Point<_VectorSpaceType> translate(const Point<_VectorSpaceType>& x, const typename _VectorSpaceType::VectorType& t) {
    Translate<Point<_VectorSpaceType>> f;
    return f(x, t);
}
#endif

} // namespace Math
} // namespace Ego
