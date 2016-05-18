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

#include "egolib/Tests/Math/MathTestUtilities.hpp"

namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
AABB<_EuclideanSpaceType> translate(const AABB<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    return AABB<_EuclideanSpaceType>(x.getMin() + t, x.getMax() + t);
}

template <typename _EuclideanSpaceType>
Cube<_EuclideanSpaceType> translate(const Cube<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    return Cube<_EuclideanSpaceType>(x.getCenter(), x.getSize());
}

template <typename _EuclideanSpaceType>
Sphere<_EuclideanSpaceType> translate(const Sphere<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    return Sphere<_EuclideanSpaceType>(x.getCenter(), x.getRadius());
}

template <typename _EuclideanSpaceType>
Line<_EuclideanSpaceType> translate(const Line<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    return Line<_EuclideanSpaceType>(x.getA() + t, x.getB() + t);
}

template <typename _EuclideanSpaceType>
Ray<_EuclideanSpaceType> translate(const Ray<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    return Ray<_EuclideanSpaceType>(x.getOrigin() + t, x.getDirection());
}

namespace Test {

EgoTest_TestCase(Translate) {
    EgoTest_Test(AABB3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::AABB3f(Point3f(-1.0f, -1.0f, -1.0f), Point3f(+1.0f, +1.0f, +1.0f));
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Cube3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Cube3f(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Sphere3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Sphere3f(Point3f(0.0f, 0.0f, 0.0f), +1.0f);
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Line3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Line3f(Point3f(-1.0f, -1.0f, -1.0f), Point3f(-1.0f, -1.0f, -1.0f));
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Ray3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Ray3f(Point3f(0.0f, 0.0f, 0.0f), Vector3f(+1.0f,+1.0f,+1.0f));
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
};

} // namespace Test
} // namespace Math
} // namespace Ego
