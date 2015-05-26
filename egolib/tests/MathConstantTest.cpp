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
#include "egolib/_math.h" /// @todo Remove this.

/**
 * @brief
 *  The error tolerance for mathematical constants.
 * @return
 *  the error tolerance
 * @remark
 *  The error between a mathematical constant and a reference may not exceed this value.
 *  Template specializations for @a float and @a double are provided.
 */
template <typename Type>
Type tolerance();

template <>
float tolerance<float>()
{
    return 1.0e-14f;
}

template <>
double tolerance<double>()
{
    return 1.0e-14;
}

EgoTest_TestCase(MathConstants) {

EgoTest_Test(piFlt)
{
    float x = Ego::Math::pi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 3.1415926535897932384626433832795f);
}

EgoTest_Test(twoPiFlt)
{
    float x = Ego::Math::twoPi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 2.0f * 3.1415926535897932384626433832795f);
}

EgoTest_Test(invPiFlt)
{
    float x = Ego::Math::invPi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 1.0f / 3.1415926535897932384626433832795f);
}

EgoTest_Test(invTwoPiFlt)
{
    float x = Ego::Math::invTwoPi<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 1.0f / (2.0f * 3.1415926535897932384626433832795f));
}

EgoTest_Test(piOverTwoFlt)
{
    float x = Ego::Math::piOverTwo<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 3.1415926535897932384626433832795f / 2.0f);
}

EgoTest_Test(piOverFourFlt)
{
    float x = Ego::Math::piOverFour<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 3.1415926535897932384626433832795f / 4.0f);
}

EgoTest_Test(piDbl)
{
    double x = Ego::Math::pi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 3.1415926535897932384626433832795);
}

EgoTest_Test(twoPiDbl)
{
    double x = Ego::Math::twoPi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 2.0 * 3.1415926535897932384626433832795);
}

EgoTest_Test(invPiDbl)
{
    double x = Ego::Math::invPi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 1.0 / 3.1415926535897932384626433832795);
}

EgoTest_Test(invTwoPiDbl)
{
    double x = Ego::Math::invTwoPi<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 1.0 / (2.0 * 3.1415926535897932384626433832795));
}

EgoTest_Test(piOverTwoDbl)
{
    double x = Ego::Math::piOverTwo<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 3.1415926535897932384626433832795 / 2.0);
}

EgoTest_Test(piOverFourDbl)
{
    double x = Ego::Math::piOverFour<double>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == 3.1415926535897932384626433832795 / 4.0);
}

EgoTest_Test(sqrtTwoFlt)
{
    float x = Ego::Math::sqrtTwo<float>();
    EgoTest_Assert(!std::isnan(x));
    EgoTest_Assert(!std::isinf(x));
    EgoTest_Assert(x == std::sqrt(2.0f));
}

EgoTest_Test(invSqrtTwoFlt)
{
    float x = Ego::Math::invSqrtTwo<float>();
    float y = 1.0f / std::sqrt(2.0f);
    EgoTest_Assert(!std::isnan(x) && !std::isnan(y));
    EgoTest_Assert(!std::isinf(x) && !std::isinf(y));
    EgoTest_Assert(0.0 < x && 0.0 < y);
    EgoTest_Assert(std::max(x, y) - std::min(x, y) <= tolerance<double>());
}

EgoTest_Test(sqrtTwoDbl)
{
    double x = Ego::Math::sqrtTwo<double>();
    double y = std::sqrt(2.0);
    EgoTest_Assert(!std::isnan(x) && !std::isnan(y));
    EgoTest_Assert(!std::isinf(x) && !std::isinf(y));
    EgoTest_Assert(0.0 < x && 0.0 < y);
    EgoTest_Assert(std::max(x,y) - std::min(x,y) <= tolerance<double>());
}

EgoTest_Test(invSqrtTwoDbl)
{
    double x = Ego::Math::invSqrtTwo<double>();
    double y = 1.0 / std::sqrt(2.0);
    // .70710678118654752440
    EgoTest_Assert(!std::isnan(x) && !std::isnan(y));
    EgoTest_Assert(!std::isinf(x) && !std::isinf(y));
    EgoTest_Assert(0.0 < x && 0.0 < y);
    EgoTest_Assert(std::max(x,y) - std::min(x,y) <= tolerance<double>());
}

};
