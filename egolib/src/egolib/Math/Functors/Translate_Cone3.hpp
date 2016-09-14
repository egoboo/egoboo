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

/// @file egolib/Math/Functors/Translate_Cone3.hpp
/// @brief Translation of cones.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/Cone3.hpp"
#include "egolib/Math/Functors/Translate.hpp"


namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
struct Translate<Cone3<_EuclideanSpaceType, void>> {
    using X = Cone3<_EuclideanSpaceType, void>;
    using T = typename _EuclideanSpaceType::VectorType;
    X operator()(const X& x, const T& t) const {
        return X(typename X::Cookie(), x.getOrigin() + t, x.getAxis(), x.getAngle());
    }
};

template <typename _EuclideanSpaceType>
Cone3<_EuclideanSpaceType, void> translate(const Cone3<_EuclideanSpaceType, void>& x, const typename _EuclideanSpaceType::VectorType& t) {
    Translate<Cone3<_EuclideanSpaceType, void>> f;
    return f(x, t);
}

} // namespace Math
} // namespace Ego
