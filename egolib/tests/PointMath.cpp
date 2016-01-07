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
#include "egolib/egolib.h"

EgoTest_DeclareTestCase(PointMath)
EgoTest_EndDeclaration()

EgoTest_BeginTestCase(PointMath)

#define TOLERANCE Point3f::ScalarType(0.0001)

EgoTest_Test(add) {
    for (size_t i = 0; i < 1000; ++i) {
        // b = a + t0
        // t1 = a - b = a - (a + t0) = -t0
        Point3f a = Point3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f t0 = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Point3f b = a + t0;
        Vector3f t1 = a - b;
        EgoTest_Assert((-t0).equalsTolerance(t1, TOLERANCE));
    }
}

EgoTest_Test(sub) {
    for (size_t i = 0; i < 1000; ++i) {
        // b = a - t0
        // t1 = b - a = (a - t0) - a = -t0
        Point3f a = Point3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f t0 = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Point3f b = a - t0;
        Vector3f t1 = b - a;
        EgoTest_Assert((-t0).equalsTolerance(t1, TOLERANCE));
    }
}

EgoTest_EndTestCase()
