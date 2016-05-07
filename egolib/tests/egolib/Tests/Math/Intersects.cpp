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

EgoTest_TestCase(Intersects) {
public:
    EgoTest_Test(AABB3f_AABB3f) {
        Ego::Math::Intersects<AABB3f, AABB3f> functor;
        AABB3f x(Point3f(-1.0f, -1.0f, -1.0f), Point3f(+1.0f, +1.0f, +1.0f));
        AABB3f y = Tests::Math::Utilities::getOverlappingAABB3f(x);
        EgoTest_Assert(functor(x, y));
        y = Tests::Math::Utilities::getNonOverlappingAABB3f(x);
        EgoTest_Assert(!functor(x, y));
    }
	
    EgoTest_Test(Cube3f_Cube3f) {
        Ego::Math::Intersects<Cube3f, Cube3f> functor;
        Cube3f x(Point3f(0.0f, 0.0f, 0.0f), 2.0);
        Cube3f y = Tests::Math::Utilities::getOverlappingCube3f(x);
        EgoTest_Assert(functor(x, y));
        y = Tests::Math::Utilities::getNonOverlappingCube3f(x);
        EgoTest_Assert(!functor(x, y));
    }
	
    EgoTest_Test(Sphere3f_Sphere3f) {
        Ego::Math::Intersects<Sphere3f, Sphere3f> functor;
        Sphere3f x(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
        Sphere3f y = Tests::Math::Utilities::getOverlappingSphere3f(x);
        EgoTest_Assert(functor(x, y));
        y = Tests::Math::Utilities::getNonOverlappingSphere3f(x);
        EgoTest_Assert(!functor(x, y));
    }

    EgoTest_Test(Point3f_Point3f) {
        Ego::Math::Intersects<Point3f, Point3f> functor;
        Point3f x(0.0f, 0.0f, 0.0f);
        Point3f y = Tests::Math::Utilities::getOverlappingPoint3f(x);
        EgoTest_Assert(functor(x, y));
        y = Tests::Math::Utilities::getNonOverlappingPoint3f(x);
        EgoTest_Assert(!functor(x, y));
    }

public:
    EgoTest_Test(AABB3f_Point3f) {
        Ego::Math::Intersects<AABB3f, Point3f> functor;
        Ego::Math::Intersects<Point3f, AABB3f> functor0;
        AABB3f x(Point3f(-1.0f, -1.0f, -1.0f), 
                 Point3f(+1.0f, +1.0f, +1.0f));
        Point3f y = Tests::Math::Utilities::getOverlappingPoint3f(x);
        EgoTest_Assert(functor(x, y) && functor0(y, x));
        y = Tests::Math::Utilities::getNonOverlappingPoint3f(x);
        EgoTest_Assert(!functor(x, y) && !functor0(y, x));
    }

    EgoTest_Test(Sphere3f_Point3f) {
        Ego::Math::Intersects<Sphere3f, Point3f> functor;
        Ego::Math::Intersects<Point3f, Sphere3f> functor0;
        Sphere3f x(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
        Point3f y = Tests::Math::Utilities::getOverlappingPoint3f(x);
        EgoTest_Assert(functor(x, y) && functor0(y, x));
        y = Tests::Math::Utilities::getNonOverlappingPoint3f(x);
        EgoTest_Assert(!functor(x, y) && !functor0(y, x));
    }
	
	EgoTest_Test(Cube3f_Point3f) {
	    Ego::Math::Intersects<Cube3f, Point3f> functor;
		Ego::Math::Intersects<Point3f, Cube3f> functor0;
        Cube3f x(Point3f(0.0f, 0.0f, 0.0f), 2.0);
        Point3f y = Tests::Math::Utilities::getOverlappingPoint3f(x);
        EgoTest_Assert(functor(x, y) && functor0(y, x));
        y = Tests::Math::Utilities::getNonOverlappingPoint3f(x);
        EgoTest_Assert(!functor(x, y) && !functor0(y, x));	
		
	}
};

}
}
} // end namespaces Ego::Math::Test
