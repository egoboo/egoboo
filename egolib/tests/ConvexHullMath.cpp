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

#include "EgoTest/EgoTest.hpp"
#include "egolib/Float.hpp"
#include "egolib/_math.h" /// @todo Remove this.

namespace Ego {
namespace Math {
namespace Test {

EgoTest_TestCase(ConvexHullTest) {

    EgoTest_Test(Sphere2Sphere) {
        auto source = Sphere3f(Vector3f(3.0f, 5.0f, 7.0f), 3.0f);
        ConvexHull<Sphere3f, Sphere3f> convex;
        auto target = convex(source);
    }

    EgoTest_Test(AABB2AABB) {
        auto source = AABB3f(Vector3f(-1.0f, -2.0f, -3.0f), Vector3f(+1.0f, +2.0f, +3.0f));
        ConvexHull<AABB3f, AABB3f> convex;
        auto target = convex(source);
    }

    EgoTest_Test(Sphere2AABB) {
        auto source = Sphere3f(Vector3f(3.0f, 5.0f, 7.0f), 3.0f);
        ConvexHull<AABB3f, Sphere3f> convex;
        auto target = convex(source);
    }

    EgoTest_Test(AAABB2Sphere) {
        auto source = AABB3f(Vector3f(-1.0f, -2.0f, -3.0f), Vector3f(+1.0f, +2.0f, 3.0f));
        ConvexHull<Sphere3f, AABB3f> convex;
        auto target = convex(source);
    }

    EgoTest_Test(Cube2AABB) {
        auto source = Cube3f(Vector3f(-1.0f, -2.0f, -3.0f), 3.0f);
        ConvexHull<AABB3f, Cube3f> convex;
        auto target = convex(source);
    }

    EgoTest_Test(AABB2Cube) {
        auto source = AABB3f(Vector3f(-1.0f, -2.0f, -3.0f), Vector3f(+1.0f, +2.0f, 3.0f));
        ConvexHull<Cube3f, AABB3f> convex;
        auto target = convex(source);
    }

};

}
}
} // end namespaces Ego::Math::Test
