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
#include "egolib/egolib.h"

EgoTest_DeclareTestCase(StringUtilities)
EgoTest_EndDeclaration()

EgoTest_BeginTestCase(StringUtilities)

EgoTest_Test(split)
{
    std::vector<std::string> v;

    v = Ego::split(std::string(""), std::string("\n"));
    if (v.size() != 0) throw std::runtime_error("");
    v.clear();

    v = Ego::split(std::string("\n"), std::string("\n"));
    if (v.size() != 1) throw std::runtime_error("");
    if (v[0] != "\n") throw std::runtime_error("");
    v.clear();

    v = Ego::split(std::string("x\ny"), std::string("\n"));
    if (v.size() != 3) throw std::runtime_error("");
    if (v[0] != "x") throw std::runtime_error("");
    if (v[1] != "\n") throw std::runtime_error("");
    if (v[2] != "y") throw std::runtime_error("");
    v.clear();

    v = Ego::split(std::string("x\n"), std::string("\n"));
    if (v.size() != 2) throw std::runtime_error("");
    if (v[0] != "x" || v[1] != "\n") throw std::runtime_error("");
    v.clear();
}

EgoTest_EndTestCase()