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

TEST(text, erase_suffix)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("Hello, World!\n");
	d.set_text(t);
	tu.connect(text_utilities::REJECT_REPLACE |
		       text_utilities::REJECT_INSERT |
		       text_utilities::REJECT_SET,
		       d);
	d.erase(std::string("Hello, ").size(), std::string("World!\n").size());
	tu.disconnect();
	ASSERT_EQ(d.get_text(), "Hello, ");
}

TEST(text, erase_prefix)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("Hello, World!\n");
	d.set_text(t);
	tu.connect(text_utilities::REJECT_REPLACE |
		       text_utilities::REJECT_INSERT |
		       text_utilities::REJECT_SET,
		       d);
	d.erase(0, std::string("Hello, ").size());
	tu.disconnect();
	ASSERT_EQ(d.get_text(), "World!\n");
}

TEST(text, erase_all)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("Hello, World!\n");
	d.set_text(t);
	tu.connect(text_utilities::REJECT_REPLACE |
		       text_utilities::REJECT_INSERT |
		       text_utilities::REJECT_SET,
		       d);
	d.erase(0, d.get_text().size());
	tu.disconnect();
	ASSERT_TRUE(d.get_text().empty());
}

TEST(text, erase_none)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("Hello, World!\n");
	for (size_t i = 0, n = d.get_text().size(); i < n; ++i)
	{
		d.set_text(t);
		tu.connect(text_utilities::REJECT_REPLACE |
			       text_utilities::REJECT_INSERT |
			       text_utilities::REJECT_ERASE |
			       text_utilities::REJECT_SET,
			       d);
		d.erase(i, 0);
		tu.disconnect();
		ASSERT_EQ(d.get_text(), t);
	}
}

} } // namespace ego::test
