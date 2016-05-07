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

EgoTest_TestCase(ColourMath) {
    
    EgoTest_Test(Lb_Lb)
    {}
    EgoTest_Test(Lf_Lf)
    {}

    EgoTest_Test(LAb_LAb)
    {}
    EgoTest_Test(LAf_LAf)
    {}

    EgoTest_Test(RGBb_RGBb)
    {}
    EgoTest_Test(RGBf_RGBf)
    {}
    
    EgoTest_Test(RGBAb_RGBAb)
    {}
    EgoTest_Test(RGBAf_RGBAf)
    {}

};

} // namespace Test
} // namespace Math
} // namespaces Ego

