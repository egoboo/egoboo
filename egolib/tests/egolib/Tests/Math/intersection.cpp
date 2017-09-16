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

TEST(intersection, axis_aligned_box_3f_axis_aligned_box_3f) {
    Ego::Math::Intersects<AxisAlignedBox3f, AxisAlignedBox3f> functor;
    auto x = AxisAlignedBox3f(Point3f(-1.0f, -1.0f, -1.0f), Point3f(+1.0f, +1.0f, +1.0f));
    auto y = Utilities::getOverlappingAxisAlignedBox3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonOverlappingAxisAlignedBox3f(x);
    ASSERT_FALSE(functor(x, y));
}
	
TEST(intersection, axis_aligned_cube_3f_axis_aligned_cube_3f) {
    Ego::Math::Intersects<AxisAlignedCube3f, AxisAlignedCube3f> functor;
    auto x = AxisAlignedCube3f(Point3f(0.0f, 0.0f, 0.0f), 2.0);
    auto y = Utilities::getOverlappingAxisAlignedCube3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonOverlappingAxisAlignedCube3f(x);
    ASSERT_FALSE(functor(x, y));
}
	
TEST(intersection, sphere_3f_sphere_3f) {
    Ego::Math::Intersects<Sphere3f, Sphere3f> functor;
    auto x = Sphere3f(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
    auto y = Utilities::getOverlappingSphere3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonOverlappingSphere3f(x);
    ASSERT_FALSE(functor(x, y));
}

TEST(intersection, point_3f_point_3f) {
    Ego::Math::Intersects<Point3f, Point3f> functor;
    auto x = Point3f(0.0f, 0.0f, 0.0f);
    auto y = Utilities::getOverlappingPoint3f(x);
    ASSERT_TRUE(functor(x, y));
    y = Utilities::getNonOverlappingPoint3f(x);
    ASSERT_FALSE(functor(x, y));
}

TEST(intersection, axis_aligned_box_3f_point_3f) {
    Ego::Math::Intersects<AxisAlignedBox3f, Point3f> functor;
    Ego::Math::Intersects<Point3f, AxisAlignedBox3f> functor0;
    auto x = AxisAlignedBox3f(Point3f(-1.0f, -1.0f, -1.0f), 
                             Point3f(+1.0f, +1.0f, +1.0f));
    auto y = Utilities::getOverlappingPoint3f(x);
    ASSERT_TRUE(functor(x, y) && functor0(y, x));
    y = Utilities::getNonOverlappingPoint3f(x);
    ASSERT_TRUE(!functor(x, y) && !functor0(y, x));
}

TEST(intersection, sphere_3f_point_3f) {
    Ego::Math::Intersects<Sphere3f, Point3f> functor;
    Ego::Math::Intersects<Point3f, Sphere3f> functor0;
    auto x = Sphere3f(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
    auto y = Utilities::getOverlappingPoint3f(x);
    ASSERT_TRUE(functor(x, y) && functor0(y, x));
    y = Utilities::getNonOverlappingPoint3f(x);
    ASSERT_TRUE(!functor(x, y) && !functor0(y, x));
}
	
TEST(intersection, axis_aligned_cube_3f_point_3f) {
	Ego::Math::Intersects<AxisAlignedCube3f, Point3f> functor;
	Ego::Math::Intersects<Point3f, AxisAlignedCube3f> functor0;
    auto x = AxisAlignedCube3f(Point3f(0.0f, 0.0f, 0.0f), 2.0);
    auto y = Utilities::getOverlappingPoint3f(x);
    ASSERT_TRUE(functor(x, y) && functor0(y, x));
    y = Utilities::getNonOverlappingPoint3f(x);
    ASSERT_TRUE(!functor(x, y) && !functor0(y, x));	
}

} } } // namespace ego::math::test
