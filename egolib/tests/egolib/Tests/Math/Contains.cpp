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

namespace ego { namespace math { namespace test {

TEST(containment, axis_aligned_box_3f_axis_aligned_box_3f) {
    Ego::Math::Contains<AxisAlignedBox3f, AxisAlignedBox3f> functor;
    auto x = AxisAlignedBox3f(Point3f(-1.0f, -1.0f, -1.0f), Point3f(+1.0f, +1.0f, +1.0f));
    auto y = Utilities::getContainedAxisAlignedBox3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonContainedAxisAlignedBox3f(x);
    ASSERT_TRUE(!functor(x, y));
}

TEST(containment, axis_aligned_box_3f_point_3f) {
    Ego::Math::Contains<AxisAlignedBox3f, Point3f> functor;
    auto x = AxisAlignedBox3f(Point3f(-1.0f, -1.0f, -1.0f), Point3f(+1.0f, +1.0f, +1.0f));
    auto y = Utilities::getContainedPoint3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonContainedPoint3f(x);
    ASSERT_TRUE(!functor(x, y));
}

TEST(containment, sphere_3f_point_3f) {
    Ego::Math::Contains<Sphere3f, Point3f> functor;
    auto x = Sphere3f(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
    auto y = Utilities::getContainedPoint3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonContainedPoint3f(x);
    ASSERT_TRUE(!functor(x, y));
}

TEST(containment, sphere_3f_sphere_3f) {
    Ego::Math::Contains<Sphere3f, Sphere3f> functor;
    auto x = Sphere3f(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
    auto y = Utilities::getContainedSphere3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonContainedSphere3f(x);
    ASSERT_TRUE(!functor(x, y));
}

TEST(containment, point_3f_point_3f) {
    Ego::Math::Contains<Point3f, Point3f> functor;
    auto x = Point3f(0.0f, 0.0f, 0.0f);
    auto y = Utilities::getContainedPoint3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonContainedPoint3f(x);
    ASSERT_TRUE(!functor(x, y));
}

} } } // namespace ego::math::test
