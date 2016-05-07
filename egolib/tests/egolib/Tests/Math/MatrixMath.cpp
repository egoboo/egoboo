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

namespace Ego {
namespace Math {
namespace Test {

EgoTest_TestCase(MatrixMath) {

EgoTest_Test(constructor) {
	Matrix4f4f a
		(
			1,  2, 3, 4,
			5,  6, 7, 8,
			9, 10,11,12,
			13,14,15,16
		);
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			EgoTest_Assert(a(i, j) == i*4+j+1);
		}
	}
}

EgoTest_Test(add) {
    Matrix4f4f a, b, c;
    c = a + b;
    EgoTest_Assert(c - b == a);
    EgoTest_Assert(c - a == b);
}

EgoTest_Test(sub) {
	Matrix4f4f a, b, c;
    c = a - b;
    EgoTest_Assert(c + b == a);
    EgoTest_Assert(b == a - c);
}

EgoTest_Test(trace) {
	Ego::Math::Matrix<float, 3, 3> m33;
	m33.trace();
}

EgoTest_Test(muls) {
	Matrix4f4f a, b;
    float s;
    do
    {
        s = Random::nextFloat();
    } while (s == 0.0f);
    b = a * s;
    EgoTest_Assert(b * (1.0f/s) == a);
}

};

} // namespace Test
} // namespace Math
} // namespace Ego
