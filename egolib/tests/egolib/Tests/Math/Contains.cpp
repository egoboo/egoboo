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

EgoTest_TestCase(Contains) {
public:
    EgoTest_Test(AABB3f_AABB3f) {
        Ego::Math::Contains<AABB3f, AABB3f> functor;
        AABB3f x(Point3f(-1.0f, -1.0f, -1.0f), Point3f(+1.0f, +1.0f, +1.0f));
        AABB3f y = Ego::Tests::Math::Utilities::getContainedAABB3f(x);
        EgoTest_Assert(functor(x, y));
        y = Ego::Tests::Math::Utilities::getNonContainedAABB3f(x);
        EgoTest_Assert(!functor(x, y));
    }

    EgoTest_Test(AABB3f_Point3f) {
        Ego::Math::Contains<AABB3f, Point3f> functor;
        AABB3f x(Point3f(-1.0f, -1.0f, -1.0f), Point3f(+1.0f, +1.0f, +1.0f));
        Point3f y = Ego::Tests::Math::Utilities::getContainedPoint3f(x);
        EgoTest_Assert(functor(x, y));
        y = Ego::Tests::Math::Utilities::getNonContainedPoint3f(x);
        EgoTest_Assert(!functor(x, y));
    }

    EgoTest_Test(Sphere3f_Point3f) {
        Ego::Math::Contains<Sphere3f, Point3f> functor;
        Sphere3f x(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
        Point3f y = Ego::Tests::Math::Utilities::getContainedPoint3f(x);
        EgoTest_Assert(functor(x, y));
        y = Ego::Tests::Math::Utilities::getNonContainedPoint3f(x);
        EgoTest_Assert(!functor(x, y));
    }

    EgoTest_Test(Sphere3f_Sphere3f) {
        Ego::Math::Contains<Sphere3f, Sphere3f> functor;
        Sphere3f x(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
        Sphere3f y = Ego::Tests::Math::Utilities::getContainedSphere3f(x);
        EgoTest_Assert(functor(x, y));
        y = Ego::Tests::Math::Utilities::getNonContainedSphere3f(x);
        EgoTest_Assert(!functor(x, y));
    }

    EgoTest_Test(Point3f_Point3f) {
        Ego::Math::Contains<Point3f, Point3f> functor;
        Point3f x(0.0f, 0.0f, 0.0f);
        Point3f y = Ego::Tests::Math::Utilities::getContainedPoint3f(x);
        EgoTest_Assert(functor(x, y));
        y = Ego::Tests::Math::Utilities::getNonContainedPoint3f(x);
        EgoTest_Assert(!functor(x, y));
    }

};

} // namespace Test
} // namespace Math
} // namespace Ego
