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

/// @file egolib/Math/Functors/Closure_AxisAlignedBox_AxisAlignedBox.hpp
/// @brief Enclose an axis aligned cubes in an axis aligned boxes.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Functors/Closure.hpp"

namespace Ego {
namespace Math {

/// Enclose an axis aligned box into an axis aligned cube.
/// The axis aligned cube is computed by selecting the component-wise minimum \f$a\f$ of the minimal
/// point component-wise maximum \f$b\f$ of the maximal point of the axis aligned box.
/// From those, the vectors \f$\vec{a}=a\vec{1}\f$ and \f$\vec{b}=b\vec{1}\f$ are computed and
/// from those the center of the cube \f$C = \vec{a} + \frac{1}{2}\left(\vec{b} - \vec{a}\right)\f$.
/// The size of the axis aligned cube is set to \f$|\vec{b}-\vec{a}|\f$.
template <typename _EuclideanSpaceType>
struct Closure<AxisAlignedCube<_EuclideanSpaceType>, AxisAlignedBox<_EuclideanSpaceType>> {
public:
    using EuclideanSpaceType = _EuclideanSpaceType;
    using SourceType = AxisAlignedBox<EuclideanSpaceType>;
    using TargetType = AxisAlignedCube<EuclideanSpaceType>;
protected:
    using VectorSpaceType = typename EuclideanSpaceType::VectorSpaceType;
    using ScalarFieldType = typename EuclideanSpaceType::ScalarFieldType;
    using GeneratorType = ConstantGenerator<typename ScalarFieldType::ScalarType>;
public:
    inline TargetType operator()(const SourceType& source) const {
        auto c = source.getCenter();
        const auto a = typename VectorSpaceType::VectorType(GeneratorType(ScalarFieldType::multiplicativeNeutral()*source.getMin().min()),
                                                            std::make_index_sequence<VectorSpaceType::VectorType::dimensionality()>{});
        const auto b = typename VectorSpaceType::VectorType(GeneratorType(ScalarFieldType::multiplicativeNeutral()*source.getMax().max()),
                                                            std::make_index_sequence<VectorSpaceType::VectorType::dimensionality()>{});
        const auto s = (b - a).length();
        return TargetType(c, s);
    }
}; // struct Closure

} // namespace Math
} // namespace Ego
