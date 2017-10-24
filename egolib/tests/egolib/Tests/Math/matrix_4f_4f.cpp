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

#include "egolib/Tests/Math/utilities.hpp"

namespace ego { namespace math { namespace test {

TEST(matrix_4f_4f, constructor) {
	Matrix4f4f a
		(
			1,  2, 3, 4,
			5,  6, 7, 8,
			9, 10,11,12,
			13,14,15,16
		);
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			ASSERT_EQ(a(i, j), i*4+j+1);
		}
	}
}

TEST(matrix_4f_4f, addition) {
    Matrix4f4f a, b, c;
    c = a + b;
    ASSERT_EQ(c - b, a);
    ASSERT_EQ(c - a, b);
}

TEST(matrix_4f_4f, subtraction) {
	Matrix4f4f a, b, c;
    c = a - b;
    ASSERT_EQ(c + b, a);
    ASSERT_EQ(b, a - c);
}

TEST(matrix_3f_3f, trace) {
	Matrix3f3f m33;
	m33.trace();
}

TEST(matrix_4f_4f, matrix_scalar_multiplication) {
	Matrix4f4f a, b;
    float s;
    do
    {
        s = Random::nextFloat();
    } while (s == 0.0f);
    b = a * s;
    ASSERT_EQ(b * (1.0f/s), a);
}

} } } // namespace ego::math::test
