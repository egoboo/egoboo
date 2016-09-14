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

/// @file egolib/Math/Functors/Translate_AxisAlignedBox.hpp
/// @brief Translation of axis aligned boxes.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/AxisAlignedBox.hpp"
#include "egolib/Math/Functors/Translate.hpp"


namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
struct Translate<AxisAlignedBox<_EuclideanSpaceType>> {
    using X = AxisAlignedBox<_EuclideanSpaceType>;
    using T = typename _EuclideanSpaceType::VectorType;
    X operator()(const X& x, const T& t) const {
        return X(x.getMin() + t, x.getMax() + t);
    }
};

template <typename _EuclideanSpaceType>
AxisAlignedBox<_EuclideanSpaceType> translate(const AxisAlignedBox<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    Translate<AxisAlignedBox<_EuclideanSpaceType>> f;
    return f(x, t);
}

} // namespace Math
} // namespace Ego
