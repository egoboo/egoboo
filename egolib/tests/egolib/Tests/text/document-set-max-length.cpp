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

	static void test_set_max_length(id::document_trim_policy trim_policy)
{
	id::document d;
	d.set_trim_policy(trim_policy);
	text_utilities tu;
	static const auto t = std::string("Hello, World!\n");
	//
	for (size_t i = 0, n = t.size(); i <= n; ++i)
	{
		int text_set_count = 0;
		int text_erased_count = 0;

		d.set_max_length(std::numeric_limits<size_t>::max());
		tu.connect(text_utilities::REJECT_REPLACE |
			       text_utilities::REJECT_INSERT, d);
		tu.m_connections.push_back(d.text_set_signal.subscribe([&text_set_count](const auto& e) {
			text_set_count++;
		}));
		tu.m_connections.push_back(d.text_erased_signal.subscribe([&text_erased_count](const auto& e) {
			text_erased_count++;
		}));
		d.set_text(t);
		d.set_max_length(i);
		tu.disconnect();
		auto required = i < t.size() ? i : t.size();
		if (i < t.size()) {
			ASSERT_EQ(text_erased_count, 1);
		}
		if (t.size() < i)
		{
			ASSERT_EQ(text_set_count, 1);
		}
		ASSERT_EQ(d.get_text().size(), required);
	}
}

TEST(text, max_text_length) {
	test_set_max_length(id::document_trim_policy::leading);
	test_set_max_length(id::document_trim_policy::trailing);
}

} } // namespace ego::test
