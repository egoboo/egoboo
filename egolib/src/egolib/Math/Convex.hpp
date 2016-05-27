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
 *  Compute a volume enclosing a given volume.
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
struct ConvexHull;

// Compute a sphere enclosing a specified axis aligned box.
template <typename _EuclideanSpaceType>
struct ConvexHull<Sphere<_EuclideanSpaceType>, AxisAlignedBox<_EuclideanSpaceType>> {
public:
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef AxisAlignedBox<_EuclideanSpaceType> SourceType;
    typedef Sphere<_EuclideanSpaceType> TargetType;
public:
    inline TargetType operator()(const SourceType& source) const {
        const auto center = source.getCenter();
        const auto radius = (source.getMin() - center).length();
        return TargetType(center, radius);
    }
};

// Compute an AABB enclosing a specified sphere.
template <typename _EuclideanSpaceType>
struct ConvexHull<AxisAlignedBox<_EuclideanSpaceType>, Sphere<_EuclideanSpaceType>> {
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
        static const auto one = typename VectorSpaceType::VectorType(GeneratorType(ScalarFieldType::multiplicativeNeutral()*radius),
                                                                     std::make_index_sequence<VectorSpaceType::VectorType::dimensionality()>{});
        const auto max = one;
        const auto min = -max;
        return TargetType(center + min, center + max);
    }
};

// Compute an axis aligned box enclosing a specified axis aligned cube.
// Let \f$min\f$ be the minimal point and \f$max\f$ be the maximal point of the axis aligned cube.
// The axis aligned box with the same minimal and maximal point is the smallest axis aligned box enclosing that axis aligned cube.
template <typename _EuclideanSpaceType>
struct ConvexHull<AxisAlignedBox<_EuclideanSpaceType>, AxisAlignedCube<_EuclideanSpaceType>> {
public:
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef AxisAlignedCube<EuclideanSpaceType> SourceType;
    typedef AxisAlignedBox<EuclideanSpaceType> TargetType;
public:
    inline TargetType operator()(const SourceType& source) const {
        return TargetType(source.getMin(), source.getMax());
    }
};

// Compute an axis aligned cube enclosing a specified axis aligned box.
// The axis aligned cube is computed by selecting the component-wise minimum \f$a\f$ of the minimal
// point component-wise maximum \f$b\f$ of the maximal point of the axis aligned box.
// From those, the vectors \f$\vec{a}=a\vec{1}\f$ and \f$\vec{b}=b\vec{1}\f$ are computed and
// from those the center of the cube \f$C = \vec{a} + \frac{1}{2}\left(\vec{b} - \vec{a}\right)\f$.
// The size of the axis aligned cube is set to \f$|\vec{b}-\vec{a}|\f$.
template <typename _EuclideanSpaceType>
struct ConvexHull<AxisAlignedCube<_EuclideanSpaceType>, AxisAlignedBox<_EuclideanSpaceType>> {
public:
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef AxisAlignedBox<EuclideanSpaceType> SourceType;
    typedef AxisAlignedCube<EuclideanSpaceType> TargetType;
protected:
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType;
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType;
    typedef ConstantGenerator<typename ScalarFieldType::ScalarType> GeneratorType;
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
};

// Compute a sphere enclosing a specified sphere (identity).
template <typename _EuclideanSpaceType>
struct ConvexHull<Sphere<_EuclideanSpaceType>, Sphere<_EuclideanSpaceType>> {
public:
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef Sphere<EuclideanSpaceType> SourceType;
    typedef Sphere<EuclideanSpaceType> TargetType;
public:
    inline TargetType operator()(const SourceType& source) const {
        return source;
    }
};

// Compute an axis aligned cube enclosing a specified axis aligned cube (identity).
template <typename _EuclideanSpaceType>
struct ConvexHull<AxisAlignedCube<_EuclideanSpaceType>, AxisAlignedCube<_EuclideanSpaceType>> {
public:
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef AxisAlignedCube<EuclideanSpaceType> SourceType;
    typedef AxisAlignedCube<EuclideanSpaceType> TargetType;
public:
    inline TargetType operator()(const SourceType& source) const {
        return source;
    }
};

// Compute an axis aligned box enclosing a specified axis aligned box (identity).
template <typename _EuclideanSpaceType>
struct ConvexHull<AxisAlignedBox<_EuclideanSpaceType>, AxisAlignedBox<_EuclideanSpaceType>> {
    typedef _EuclideanSpaceType EuclideanSpaceType;
    typedef AxisAlignedBox<_EuclideanSpaceType> SourceType;
    typedef AxisAlignedBox<_EuclideanSpaceType> TargetType;
public:
    inline TargetType operator()(const SourceType& source) const {
        return source;
    }
};

} // namespace Math
} // namespace Ego
