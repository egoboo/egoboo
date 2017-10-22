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

#include "egolib/tests/text/utilities.hpp"

namespace ego { namespace test {

TEST(text, prepend)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string(", World!\n");
	d.set_text(t);
	tu.connect(text_utilities::REJECT_ERASE | text_utilities::REJECT_SET | text_utilities::REJECT_REPLACE,
		       d);
	d.prepend_text("Hello");
	tu.disconnect();
	ASSERT_EQ(d.get_text(), "Hello, World!\n");
}

TEST(text, append)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("Hello, ");
	d.set_text(t);
	tu.connect(text_utilities::REJECT_ERASE|text_utilities::REJECT_SET|text_utilities::REJECT_REPLACE,
		       d);
	d.append_text("World!\n");
	tu.disconnect();
	ASSERT_EQ(d.get_text(), "Hello, World!\n");
}

TEST(text, insert)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("HelloWorld!\n");
	d.set_text(t);
	tu.connect(text_utilities::REJECT_ERASE | text_utilities::REJECT_SET | text_utilities::REJECT_REPLACE,
		       d);
	d.insert_text(", ", 5);
	tu.disconnect();
	ASSERT_EQ(d.get_text(), "Hello, World!\n");
}

} } // namespace ego::test
