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

namespace ego { namespace math { namespace test {

TEST(constants, pi_f) {
    auto x = Ego::Math::pi<float>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 3.1415926535897932384626433832795f, 2));
}

TEST(constants, two_pi_f) {
    auto x = Ego::Math::twoPi<float>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 2.0f * 3.1415926535897932384626433832795f, 2));
}

TEST(constants, inv_pi_f) {
    auto x = Ego::Math::invPi<float>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 1.0f / 3.1415926535897932384626433832795f, 2));
}

TEST(constants, inv_two_pi_f) {
    auto x = Ego::Math::invTwoPi<float>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 1.0f / (2.0f * 3.1415926535897932384626433832795f), 2));
}

TEST(constants, pi_over_two_f) {
    auto x = Ego::Math::piOverTwo<float>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 3.1415926535897932384626433832795f / 2.0f, 2));
}

TEST(constants, pi_over_four_f) {
    auto x = Ego::Math::piOverFour<float>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 3.1415926535897932384626433832795f / 4.0f, 2));
}

TEST(constants, pi_d) {
    auto x = Ego::Math::pi<double>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 3.1415926535897932384626433832795, 2));
}

TEST(constants, two_pi_d) {
    auto x = Ego::Math::twoPi<double>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 2.0 * 3.1415926535897932384626433832795, 2));
}

TEST(constants, inv_pi_d) {
    auto x = Ego::Math::invPi<double>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 1.0 / 3.1415926535897932384626433832795, 2));
}

TEST(constants, inv_two_pi_d) {
    auto x = Ego::Math::invTwoPi<double>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 1.0 / (2.0 * 3.1415926535897932384626433832795), 2));
}

TEST(constants, pi_over_two_d) {
    auto x = Ego::Math::piOverTwo<double>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 3.1415926535897932384626433832795 / 2.0, 2));
}

TEST(constants, pi_over_four_d) {
    auto x = Ego::Math::piOverFour<double>();
    ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, 3.1415926535897932384626433832795 / 4.0, 2));
}

TEST(constants, sqrt_two_f) {
    auto x = Ego::Math::sqrtTwo<float>();
	ASSERT_TRUE(!std::isnan(x));
	ASSERT_TRUE(!std::isinf(x));
	ASSERT_TRUE(float_equalToUlp(x, std::sqrt(2.0f), 2));
}

TEST(constants, inv_sqrt_two_f) {
    auto x = Ego::Math::invSqrtTwo<float>();
    auto y = 1.0f / std::sqrt(2.0f);
	ASSERT_TRUE(!std::isnan(x) && !std::isnan(y));
	ASSERT_TRUE(!std::isinf(x) && !std::isinf(y));
	ASSERT_TRUE(0.0 < x && 0.0 < y);
	ASSERT_TRUE(float_equalToUlp(x, y, 2));
}

TEST(constants, sqrt_two_d) {
    auto x = Ego::Math::sqrtTwo<double>();
    auto y = std::sqrt(2.0);
	ASSERT_TRUE(!std::isnan(x) && !std::isnan(y));
	ASSERT_TRUE(!std::isinf(x) && !std::isinf(y));
	ASSERT_TRUE(0.0 < x && 0.0 < y);
	ASSERT_TRUE(float_equalToUlp(x, y, 2));
}

TEST(constants, inv_sqrt_two_d) {
    double x = Ego::Math::invSqrtTwo<double>();
    double y = 1.0 / std::sqrt(2.0);
    // .70710678118654752440
	ASSERT_TRUE(!std::isnan(x) && !std::isnan(y));
	ASSERT_TRUE(!std::isinf(x) && !std::isinf(y));
	ASSERT_TRUE(0.0 < x && 0.0 < y);
	ASSERT_TRUE(float_equalToUlp(x, y, 2));
}
  
} } } // namespace ego::math::test
