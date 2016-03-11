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
#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Math {
namespace Test {

EgoTest_TestCase(ColourMathTest) {
    
    EgoTest_Test(Lb2Lb)
    {}
    EgoTest_Test(Lf2Lf)
    {}

    EgoTest_Test(LAb2LAb)
    {}
    EgoTest_Test(LAf2LAf)
    {}

    EgoTest_Test(RGBb2RGBb)
    {}
    EgoTest_Test(RGBf2RGBf)
    {}
    
    EgoTest_Test(RGBAb2RGBAb)
    {}
    EgoTest_Test(RGBAf2RGBAf)
    {}

};

}
}
} // end namespaces Ego::Math::Test
