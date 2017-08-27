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

static void test(const Vector3f& x, const Vector3f& y) {
	auto f = id::interpolation_functor<Vector3f, id::interpolation_method::LINEAR>();
	ASSERT_EQ(x, f(x, y, 0.0f));
	ASSERT_EQ(y, f(x, y, 1.0f));
}

TEST(linear_interpolation, vector_3f) {
	auto l = Utilities::zero(Utilities::negation(Utilities::basis(std::vector<Vector3f>())));
	auto cl = Utilities::cartesian(l, l);
	for (auto c : cl) {
		test(c.first, c.second);
	}
}

} } } // namespace ego::math::test
