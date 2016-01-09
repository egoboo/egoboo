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

namespace Ego { namespace Math { namespace Test {

EgoTest_TestCase(MathConstants)
{

EgoTest_Test(piFlt)
{
    float x = Ego::Math::pi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 3.1415926535897932384626433832795f, 2));
}

EgoTest_Test(twoPiFlt)
{
    float x = Ego::Math::twoPi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 2.0f * 3.1415926535897932384626433832795f, 2));
}

EgoTest_Test(invPiFlt)
{
    float x = Ego::Math::invPi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 1.0f / 3.1415926535897932384626433832795f, 2));
}

EgoTest_Test(invTwoPiFlt)
{
    float x = Ego::Math::invTwoPi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 1.0f / (2.0f * 3.1415926535897932384626433832795f), 2));
}

EgoTest_Test(piOverTwoFlt)
{
    float x = Ego::Math::piOverTwo<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 3.1415926535897932384626433832795f / 2.0f, 2));
}

EgoTest_Test(piOverFourFlt)
{
    float x = Ego::Math::piOverFour<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 3.1415926535897932384626433832795f / 4.0f, 2));
}

EgoTest_Test(piDbl)
{
    double x = Ego::Math::pi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 3.1415926535897932384626433832795, 2));
}

EgoTest_Test(twoPiDbl)
{
    double x = Ego::Math::twoPi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 2.0 * 3.1415926535897932384626433832795, 2));
}

EgoTest_Test(invPiDbl)
{
    double x = Ego::Math::invPi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 1.0 / 3.1415926535897932384626433832795, 2));
}

EgoTest_Test(invTwoPiDbl)
{
    double x = Ego::Math::invTwoPi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 1.0 / (2.0 * 3.1415926535897932384626433832795), 2));
}

EgoTest_Test(piOverTwoDbl)
{
    double x = Ego::Math::piOverTwo<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 3.1415926535897932384626433832795 / 2.0, 2));
}

EgoTest_Test(piOverFourDbl)
{
    double x = Ego::Math::piOverFour<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, 3.1415926535897932384626433832795 / 4.0, 2));
}

EgoTest_Test(sqrtTwoFlt)
{
    float x = Ego::Math::sqrtTwo<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(float_equalToUlp(x, std::sqrt(2.0f), 2));
}

EgoTest_Test(invSqrtTwoFlt)
{
    float x = Ego::Math::invSqrtTwo<float>();
    float y = 1.0f / std::sqrt(2.0f);
    EgoTest_Assert(!std::isnan(x) && !std::isnan(y));
    EgoTest_Assert(!std::isinf(x) && !std::isinf(y));
    EgoTest_Assert(0.0 < x && 0.0 < y);
    EgoTest_Assert(float_equalToUlp(x, y, 2));
}

EgoTest_Test(sqrtTwoDbl)
{
    double x = Ego::Math::sqrtTwo<double>();
    double y = std::sqrt(2.0);
    EgoTest_Assert(!std::isnan(x) && !std::isnan(y));
    EgoTest_Assert(!std::isinf(x) && !std::isinf(y));
    EgoTest_Assert(0.0 < x && 0.0 < y);
    EgoTest_Assert(float_equalToUlp(x, y, 2));
}

EgoTest_Test(invSqrtTwoDbl)
{
    double x = Ego::Math::invSqrtTwo<double>();
    double y = 1.0 / std::sqrt(2.0);
    // .70710678118654752440
    EgoTest_Assert(!std::isnan(x) && !std::isnan(y));
    EgoTest_Assert(!std::isinf(x) && !std::isinf(y));
    EgoTest_Assert(0.0 < x && 0.0 < y);
    EgoTest_Assert(float_equalToUlp(x, y, 2));
}

};
    
} } } // end namespaces Ego::Math::Test
