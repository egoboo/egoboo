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

// This is an example for EgoTest

#include "EgoTest/EgoTest.hpp"

#include "vec2.hpp"

EgoTest_DeclareTestCase(Vec2Test)
EgoTest_EndDeclaration()

EgoTest_BeginTestCase(Vec2Test)

EgoTest_Test(lengthTest)
{
    fvec2_t test(15, 42);
    EgoTest_Assert(test.length() == 44.59820624f);
}

EgoTest_Test(normalizeToLengthTest)
{
    fvec2_t test(27, 36);
    fvec2_t expected(3, 4);
    test.normalize(5);
    EgoTest_Assert(test.equals(expected));
}

EgoTest_Test(normalizeTest)
{
    fvec2_t test(1, 1);
    float one_over_sqrt2 = 1.f / std::sqrt(2.f);
    fvec2_t expected(one_over_sqrt2, one_over_sqrt2);
    test.normalize();
    EgoTest_Assert(test.equals(expected));
}

EgoTest_Test(dotTest)
{
    fvec2_t test1(30, 10);
    fvec2_t test2(12, 19);
    EgoTest_Assert(test1.dot(test2) == test2.dot(test1));
    EgoTest_Assert(test1.dot(test2) == 550);
    test1 = fvec2_t(-5, 0);
    test2 = fvec2_t(0, 8);
    EgoTest_Assert(test1.dot(test2) == 0);
}

EgoTest_Test(multiplyTest)
{
    fvec2_t test(3, -14);
    fvec2_t expected(4.5, -21);
    test.multiply(1.5f);
    EgoTest_Assert(test.equals(expected));
}

EgoTest_Test(lengthSquaredTest)
{
    fvec2_t test(6, 8);
    EgoTest_Assert(test.length_2() == 100);
}

EgoTest_Test(lengthTaxiTest)
{
    fvec2_t test(500, -203);
    EgoTest_Assert(test.length_abs() == 703);
}

EgoTest_Test(addTest)
{
    fvec2_t test(501, -956);
    test += fvec2_t(-51, 528);
    fvec2_t expected(450, -428);
    EgoTest_Assert(test.equals(expected));
}

EgoTest_Test(subTest)
{
    fvec2_t test(501, -956);
    test -= fvec2_t(-51, 528);
    fvec2_t expected(552, -1484);
    EgoTest_Assert(test.equals(expected));
}

EgoTest_Test(accessTest)
{
    fvec2_t test(50123, -21913);
    EgoTest_Assert(test.x == test.v[0]);
    EgoTest_Assert(test.x == test.s);
    EgoTest_Assert(test.x == test[0]);
    EgoTest_Assert(test.x == 50123);
    
    EgoTest_Assert(test.y == test.v[1]);
    EgoTest_Assert(test.y == test.t);
    EgoTest_Assert(test.y == test[1]);
    EgoTest_Assert(test.y == -21913);
}

EgoTest_EndTestCase()