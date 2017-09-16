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

#include "MathTestUtilities.hpp"

namespace ego { namespace math { namespace test {

#define TOLERANCE Vector3f::ScalarType(0.0001)

TEST(vector_3f, addition) {
    for (size_t i = 0; i < 1000; ++i) {
        auto a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        auto b = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        auto c = a + b;
        ASSERT_TRUE((c - b).equalsTolerance(a, TOLERANCE));
        ASSERT_TRUE((c - a).equalsTolerance(b, TOLERANCE));
    }
}

TEST(vector_3f, subtraction) {
    for (size_t i = 0; i < 1000; ++i) {
        auto a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        auto b = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        auto c = a - b;
        ASSERT_TRUE((c + b).equalsTolerance(a, TOLERANCE));
        ASSERT_TRUE(b.equalsTolerance(a - c, TOLERANCE));
    }
}

TEST(vector_3f, scalar_product) {
    for (size_t i = 0; i < 1000; ++i) {
        auto a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        auto b = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        float s;
        do {
            s = Random::nextFloat();
        } while (s == 0.0f);
        b = a * s;
        ASSERT_TRUE((b * (1.0f / s)).equalsTolerance(a, TOLERANCE));
    }
}

TEST(vector_3f, negation) {
    for (size_t i = 0; i < 1000; ++i) {
        auto a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        auto b = -a;
        auto c = -b;
        ASSERT_TRUE(a.equalsTolerance(c, TOLERANCE));
    }
}

TEST(vector_3f, equality) {
    for (size_t i = 0; i < 1000; ++i) {
        Vector3f a = Vector3f(Random::nextFloat(), Random::nextFloat(), Random::nextFloat());
        Vector3f b = a;
        ASSERT_EQ(a, b);
    }
}

TEST(vector_3f, length) {
    auto zero = Vector3f::zero();
    ASSERT_TRUE(zero[0] == 0.0f && zero[1] == 0.0f && zero[2] == 0.0f);
    auto x = Vector3f::unit(0); 
    ASSERT_TRUE(x.length() - 1.0f <= TOLERANCE);
    ASSERT_TRUE(x[1] == 0.0f && x[2] == 0.0f);
    auto y = Vector3f::unit(1);
    ASSERT_TRUE(y.length() - 1.0f <= TOLERANCE);
    ASSERT_TRUE(y[0] == 0.0f && y[2] == 0.0f);
    auto z = Vector3f::unit(2);
    ASSERT_TRUE(z.length() - 1.0f <= TOLERANCE);
    ASSERT_TRUE(z[0] == 0.0f && z[1] == 0.0f);
}

} } } // namespace ego::math::test
