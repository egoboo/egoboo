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

EgoTest_TestCase(StringUtilities)
{

EgoTest_Test(textFormatter)
{
    std::string input, output;

    input = "How many G'nomes does it take to change a Light Bulb? None, they can't figure out how to change one with gonnepowder or grog.";
    output = add_linebreak_cpp(input, 50);
    EgoTest_Assert(output == "How many G'nomes does it take to change a Light\nBulb? None, they can't figure out how to change\none with gonnepowder or grog.");

    input = "1234567 cf qos a";
    output = add_linebreak_cpp(input, 6);
    EgoTest_Assert(output == "1234567\ncf qos\na");
}

EgoTest_Test(left_trim_ws)
{
    std::string input = " \tfo \to \t";
    std::string output = Ego::left_trim_ws(input);
    EgoTest_Assert(output == "fo \to \t");
}

EgoTest_Test(right_trim_ws)
{
    std::string input = " \tfo \to \t";
    std::string output = Ego::right_trim_ws(input);
    EgoTest_Assert(output == " \tfo \to");
}

EgoTest_Test(trim_ws)
{
    std::string input = " \tfo \to \t";
    std::string output = Ego::trim_ws(input);
    EgoTest_Assert(output == "fo \to");
}

EgoTest_Test(split)
{
    std::vector<std::string> v;

    v = Ego::split(std::string(""), std::string("\n"));
    EgoTest_Assert(0 == v.size());
    v.clear();

    v = Ego::split(std::string("\n"), std::string("\n"));
    EgoTest_Assert(1 == v.size());
    EgoTest_Assert("\n" == v[0]);
    v.clear();

    v = Ego::split(std::string("x\ny"), std::string("\n"));
    EgoTest_Assert(3 == v.size());
    EgoTest_Assert("x" == v[0]);
    EgoTest_Assert("\n" == v[1]);
    EgoTest_Assert("y" == v[2]);
    v.clear();

    v = Ego::split(std::string("x\n"), std::string("\n"));
    EgoTest_Assert(2 == v.size());
    EgoTest_Assert("x" == v[0] && "\n" == v[1]);
    v.clear();
}

EgoTest_Test(toupper)
{
    std::string x("x");
    Ego::toupper(x);
    EgoTest_Assert(x == "X");
    // Identity.
    Ego::toupper(x);
    EgoTest_Assert(x == "X");
}

EgoTest_Test(tolower)
{
    std::string x("X");
    Ego::tolower(x);
    EgoTest_Assert(x == "x");
    // Identity.
    Ego::tolower(x);
    EgoTest_Assert(x == "x");
}

};
