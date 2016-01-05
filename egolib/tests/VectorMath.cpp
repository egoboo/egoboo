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

EgoTest_DeclareTestCase(VectorMath)
EgoTest_EndDeclaration()

EgoTest_BeginTestCase(VectorMath)

#define TOLERANCE Vector3f::ScalarType(0.0001)

EgoTest_Test(add)
{
    for (size_t i = 0; i < 1000; ++i) {
        Vector3f a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f b = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f c = a + b;
        if (!(c - b).equalsTolerance(a, TOLERANCE))
        {
            EgoTest_Assert((c - b).equalsTolerance(a, TOLERANCE));
        }
        if (!(c - a).equalsTolerance(b, TOLERANCE)) {
            EgoTest_Assert((c - a).equalsTolerance(b, TOLERANCE));
        }
    }
}

EgoTest_Test(sub)
{
    for (size_t i = 0; i < 1000; ++i) {
        Vector3f a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f b = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f c = a - b;
        EgoTest_Assert((c + b).equalsTolerance(a, TOLERANCE));
        EgoTest_Assert(b.equalsTolerance(a - c, TOLERANCE));
    }
}

EgoTest_Test(muls)
{
    for (size_t i = 0; i < 1000; ++i) {
        Vector3f a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f b = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        float s;
        do {
            s = Random::nextFloat();
        } while (s == 0.0f);
        b = a * s;
        EgoTest_Assert((b * (1.0f / s)).equalsTolerance(a, TOLERANCE));
    }
}

EgoTest_Test(neg)
{
    for (size_t i = 0; i < 1000; ++i) {
        Vector3f a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f b = -a;
        Vector3f c = -b;
        EgoTest_Assert(a.equalsTolerance(c, TOLERANCE));
    }
}

EgoTest_Test(length) {
    Vector3f zero = Vector3f::zero();
    EgoTest_Assert(zero[0] == 0.0f && zero[1] == 0.0f && zero[2] == 0.0f);
    Vector3f x = Vector3f::unit(0); 
    EgoTest_Assert(x.length() - 1.0f <= TOLERANCE);
    EgoTest_Assert(x[1] == 0.0f && x[2] == 0.0f);
    Vector3f y = Vector3f::unit(1);
    EgoTest_Assert(y.length() - 1.0f <= TOLERANCE);
    EgoTest_Assert(y[0] == 0.0f && y[2] == 0.0f);
    Vector3f z = Vector3f::unit(2);
    EgoTest_Assert(z.length() - 1.0f <= TOLERANCE);
    EgoTest_Assert(z[0] == 0.0f && z[1] == 0.0f);
}

EgoTest_EndTestCase()
