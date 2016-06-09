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
namespace Test {

EgoTest_TestCase(Closure) {

    EgoTest_Test(Sphere3f_Sphere3f) {
        auto source = Sphere3f(Point3f(3.0f, 5.0f, 7.0f), 3.0f);
        Ego::Math::Closure<Sphere3f, Sphere3f> closure;
        auto target = closure(source);
    }

    EgoTest_Test(AABB3f_AABB3f) {
        auto source = AxisAlignedBox3f(Point3f(-1.0f, -2.0f, -3.0f), Point3f(+1.0f, +2.0f, +3.0f));
        Ego::Math::Closure<AxisAlignedBox3f, AxisAlignedBox3f> closure;
        auto target = closure(source);
    }

    EgoTest_Test(Sphere3f_AABB3f) {
        auto source = Sphere3f(Point3f(3.0f, 5.0f, 7.0f), 3.0f);
        Ego::Math::Closure<AxisAlignedBox3f, Sphere3f> closure;
        auto target = closure(source);
    }

    EgoTest_Test(AxisAlignedBox3f_Sphere3f) {
        auto source = AxisAlignedBox3f(Point3f(-1.0f, -2.0f, -3.0f), Point3f(+1.0f, +2.0f, 3.0f));
        Ego::Math::Closure<Sphere3f, AxisAlignedBox3f> closure;
        auto target = closure(source);
    }

    EgoTest_Test(AxisAlignedCube3f_AxisAlignedBox3f) {
        auto source = AxisAlignedCube3f(Point3f(-1.0f, -2.0f, -3.0f), 3.0f);
        Ego::Math::Closure<AxisAlignedBox3f, AxisAlignedCube3f> closure;
        auto target = closure(source);
    }

    EgoTest_Test(AxisAlignedBox3f_AxisAlignedCube3f) {
        auto source = AxisAlignedBox3f(Point3f(-1.0f, -2.0f, -3.0f), Point3f(+1.0f, +2.0f, 3.0f));
        Ego::Math::Closure<AxisAlignedCube3f, AxisAlignedBox3f> closure;
        auto target = closure(source);
    }

};

}
}
} // end namespaces Ego::Math::Test
