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

#define TOLERANCE fvec3_t::ScalarType(0.0001)

EgoTest_Test(add)
{
    fvec3_t a, b, c;
    for (size_t i = 0; i < 1000; ++i) {
        a = fvec3_t(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        b = fvec3_t(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        c = a + b;
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
    fvec3_t a, b, c;
    for (size_t i = 0; i < 1000; ++i) {
        a = fvec3_t(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        b = fvec3_t(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        c = a - b;
        EgoTest_Assert((c + b).equalsTolerance(a, TOLERANCE));
        EgoTest_Assert(b.equalsTolerance(a - c, TOLERANCE));
    }
}

EgoTest_Test(muls)
{
    fvec3_t a, b, c;
    for (size_t i = 0; i < 1000; ++i) {
        a = fvec3_t(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        b = fvec3_t(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        float s;
        do {
            s = Random::nextFloat();
        } while (s == 0.0f);
        b = a * s;
        EgoTest_Assert((b * (1.0f / s)).equalsTolerance(a, TOLERANCE));
    }
}

EgoTest_EndTestCase()
