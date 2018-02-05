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

TEST(text, append_console_style_1)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("xxxx");
	d.set_text(t);
	d.set_max_length(4);
	tu.connect(text_utilities::REJECT_ERASE | text_utilities::REJECT_SET | text_utilities::REJECT_REPLACE, d);
	d.append_text("yyyy");
	tu.disconnect();
	ASSERT_EQ(d.get_text(), "yyyy");
}

TEST(text, append_console_style_2)
{
	id::document d;
	text_utilities tu;
	static const auto t = std::string("xxxx");
	d.set_text(t);
	d.set_trim_policy(id::document_trim_policy::leading);
	d.set_max_length(6);
	tu.connect(text_utilities::REJECT_ERASE | text_utilities::REJECT_SET | text_utilities::REJECT_REPLACE, d);
	d.append_text("yyyy");
	tu.disconnect();
	ASSERT_EQ(d.get_text(), "xxyyyy");
}


} } // namespace ego::test
