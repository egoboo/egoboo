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

/// @file egolib/Math/Functors/Closure_AxisAlignedBox_Sphere.hpp
/// @brief Enclose spheres in axis aligned boxes.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Closure.hpp"

namespace Ego {
namespace Math {

/// Enclose a sphere into an axis aligned box.
template <typename _EuclideanSpaceType>
struct Closure<AxisAlignedBox<_EuclideanSpaceType>, Sphere<_EuclideanSpaceType>> {
public:
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef Sphere<EuclideanSpaceType> SourceType;
    typedef AxisAlignedBox<EuclideanSpaceType> TargetType;
protected:
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType;
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType;
    typedef ConstantGenerator<typename ScalarFieldType::ScalarType> GeneratorType;
public:
    inline TargetType operator()(const SourceType& source) const {
        const auto center = source.getCenter();
        const auto radius = source.getRadius();
        static const auto extend = typename VectorSpaceType::VectorType(GeneratorType(ScalarFieldType::multiplicativeNeutral()),
                                                                        std::make_index_sequence<VectorSpaceType::VectorType::dimensionality()>{}) * radius;
        return TargetType(center - extend, center + extend);
    }
};
} // namespace Math
} // namespace Ego
