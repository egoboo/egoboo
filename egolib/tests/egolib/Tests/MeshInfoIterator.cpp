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

#include "gtest/gtest.h"
#include "egolib/Mesh/Info.hpp"

namespace Ego { namespace Test {

TEST(mesh_info_iterator_test, test_zero_elements_1)
{
	auto info = Ego::MeshInfo(8, 8, 0);
	for (auto it = info.begin(); it != info.end(); ++it)
	{
		ASSERT_FALSE(false);
	}
}

TEST(mesh_info_iterator_test, test_zero_elements_2)
{
    auto info = Ego::MeshInfo(8, 0, 8);
    for (auto it = info.begin(); it != info.end(); ++it)
    {
        ASSERT_FALSE(false);
    }
}

TEST(mesh_info_iterator_test, test_one_element_1)
{
    std::vector<Index2D> v{{0,0}};
    auto info = Ego::MeshInfo(8, 1, 1);
    int i = 0;
    for (auto it = info.begin(); it != info.end(); ++it)
    {
        ASSERT_EQ(*it, v[i]);
        i++;
    }
}

} } // namespace Ego::Test
