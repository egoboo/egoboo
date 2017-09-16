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

#include "MathTestUtilities.hpp"

namespace ego { namespace math { namespace test {

TEST(point_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = Point3f(-1.0f, -1.0f, -1.0f);
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

TEST(axis_aligned_box_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = AxisAlignedBox3f(Point3f(-1.0f, -1.0f, -1.0f), ::Point3f(+1.0f, +1.0f, +1.0f));
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

TEST(axis_aligned_cube_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = AxisAlignedCube3f(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

TEST(sphere_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = Sphere3f(Point3f(0.0f, 0.0f, 0.0f), +1.0f);
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

TEST(line_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = Line3f(Point3f(-1.0f, -1.0f, -1.0f), Point3f(-1.0f, -1.0f, -1.0f));
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

TEST(ray_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = Ray3f(Point3f(0.0f, 0.0f, 0.0f), Vector3f(+1.0f,+1.0f,+1.0f));
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

TEST(cone_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = Cone3f();
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

TEST(plane_3f, translate) {
    auto t = Vector3f(+1.0f, +1.0f, +1.0f);
    auto x = Plane3f();
    auto y = translate(x, t);
    y = translate(y, -t);
    ASSERT_EQ(x, y);
}

} } } // namespace ego::math::test
