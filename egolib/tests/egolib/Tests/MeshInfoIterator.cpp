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
#include "egolib/Mesh/Info.hpp"

namespace Ego {
namespace Test {

EgoTest_TestCase(MeshInfoIterator)
{
    EgoTest_Test(zeroElements0)
    {
        auto info = Ego::MeshInfo(8, 8, 0);
        for (auto it = info.begin(); it != info.end(); ++it)
        {
            EgoTest_Assert(false);
        }
    }
    EgoTest_Test(zeroElements1)
    {
        auto info = Ego::MeshInfo(8, 0, 8);
        for (auto it = info.begin(); it != info.end(); ++it)
        {
            EgoTest_Assert(false);
        }
    }
    EgoTest_Test(oneElement0)
    {
        std::vector<Index2D> v{{0,0}};
        auto info = Ego::MeshInfo(8, 1, 1);
        int i = 0;
        for (auto it = info.begin(); it != info.end(); ++it)
        {
            EgoTest_Assert(*it == v[i]);
            i++;
        }
    }
};

} // namespace Test
} // namespace Ego
