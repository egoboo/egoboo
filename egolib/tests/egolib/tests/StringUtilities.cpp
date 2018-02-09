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
#include "egolib/egolib.h"

TEST(string_utilities, text_formatting)
{
    std::string input, output;

    input = "How many G'nomes does it take to change a Light Bulb? None, they can't figure out how to change one with gonnepowder or grog.";
    output = add_linebreak_cpp(input, 50);
	ASSERT_EQ(output, "How many G'nomes does it take to change a Light\nBulb? None, they can't figure out how to change\none with gonnepowder or grog.");

    input = "1234567 cf qos a";
    output = add_linebreak_cpp(input, 6);
	ASSERT_EQ(output, "1234567\ncf qos\na");
}

TEST(string_utilities, left_trim_ws)
{
    std::string input = " \tfo \to \t";
    std::string output = Ego::left_trim_ws(input);
	ASSERT_EQ(output, "fo \to \t");
}

TEST(string_utilities, right_trim_ws)
{
    std::string input = " \tfo \to \t";
    std::string output = Ego::right_trim_ws(input);
	ASSERT_EQ(output, " \tfo \to");
}

TEST(string_utilities, trim_ws)
{
    std::string input = " \tfo \to \t";
    std::string output = Ego::trim_ws(input);
	ASSERT_EQ(output, "fo \to");
}

TEST(string_utilities, split)
{
    std::vector<std::string> v;

    v = Ego::split(std::string(""), std::string("\n"));
	ASSERT_EQ(0, v.size());
    v.clear();

    v = Ego::split(std::string("\n"), std::string("\n"));
	ASSERT_EQ(1, v.size());
	ASSERT_EQ("\n", v[0]);
    v.clear();

    v = Ego::split(std::string("x\ny"), std::string("\n"));
	ASSERT_EQ(3, v.size());
	ASSERT_EQ("x", v[0]);
	ASSERT_EQ("\n", v[1]);
	ASSERT_EQ("y", v[2]);
    v.clear();

    v = Ego::split(std::string("x\n"), std::string("\n"));
    ASSERT_EQ(2, v.size());
    ASSERT_TRUE("x" == v[0] && "\n" == v[1]);
    v.clear();
}
