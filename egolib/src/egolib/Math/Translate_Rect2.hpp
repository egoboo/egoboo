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

/// @file egolib/Math/Translate_Rect2.hpp
/// @brief Translation of 2D rectangles.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Rect2.hpp"

namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
struct Translate<Rect2<_EuclideanSpaceType, void>> {
    typedef Rect2<_EuclideanSpaceType, void> X;
    typedef typename _EuclideanSpaceType::VectorType T;
    X operator()(const X& x, const T& t) {
        return X(x.getCenter() + t, x.getSize());
    }
};

template <typename _EuclideanSpaceType>
Rect2<_EuclideanSpaceType, void> translate(const Rect2<_EuclideanSpaceType, void>& x, const typename _EuclideanSpaceType::VectorType& t) {
    Translate<Rect2<_EuclideanSpaceType, void>> f;
    return f(x, t);
}

} // namespace Math
} // namespace Ego
