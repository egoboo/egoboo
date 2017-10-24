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

#include "egolib/Tests/Math/utilities.hpp"

namespace ego { namespace math { namespace test {

TEST(enclose, sphere_3f_sphere_3f) {
	auto source = Sphere3f(Point3f(3.0f, 5.0f, 7.0f), 3.0f);
	auto target = id::enclose<Sphere3f>(source);
}

TEST(enclose, aabb_3f_aabb_3f) {
    auto source = AxisAlignedBox3f(Point3f(-1.0f, -2.0f, -3.0f), Point3f(+1.0f, +2.0f, +3.0f));
    auto target = id::enclose<AxisAlignedBox3f>(source);
}

TEST(enclose, sphere_3f_aabb_3f) {
    auto source = Sphere3f(Point3f(3.0f, 5.0f, 7.0f), 3.0f);
    auto target = id::enclose<AxisAlignedBox3f>(source);
}

TEST(enclose, axis_aligned_box_3f_sphere_3f) {
    auto source = AxisAlignedBox3f(Point3f(-1.0f, -2.0f, -3.0f), Point3f(+1.0f, +2.0f, 3.0f));
    auto target = id::enclose<Sphere3f>(source);
}

TEST(enclose, axis_aligned_cube_3f_axis_aligned_box_3f) {
    auto source = AxisAlignedCube3f(Point3f(-1.0f, -2.0f, -3.0f), 3.0f);
    auto target = id::enclose<AxisAlignedBox3f>(source);
}

TEST(enclose, axis_aligned_box_3f_axis_aligned_cube_3f) {
    auto source = AxisAlignedBox3f(Point3f(-1.0f, -2.0f, -3.0f), Point3f(+1.0f, +2.0f, 3.0f));
    auto target = id::enclose<AxisAlignedCube3f>(source);
}

} } } // namespace ego::math::test
