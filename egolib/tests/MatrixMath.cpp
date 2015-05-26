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

EgoTest_TestCase(MatrixMath) {

EgoTest_Test(add)
{
    fmat_4x4_t a, b, c;
    c = a + b;
    EgoTest_Assert(c - b == a);
    EgoTest_Assert(c - a == b);
}

EgoTest_Test(sub)
{
    fmat_4x4_t a, b, c;
    c = a - b;
    EgoTest_Assert(c + b == a);
    EgoTest_Assert(b == a - c);
}

EgoTest_Test(muls)
{
    fmat_4x4_t a, b;
    float s;
    do
    {
        s = Random::nextFloat();
    } while (s == 0.0f);
    b = a * s;
    EgoTest_Assert(b * (1.0f/s) == a);
}

};
