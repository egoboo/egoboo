#pragma once

#include "egolib/text/document.hpp"
#include "gtest/gtest.h"

namespace ego { namespace test {
struct text_utilities
{
	enum reject
	{
		REJECT_INSERT = 1,
		REJECT_REPLACE = 2,
		REJECT_ERASE = 4,
		REJECT_SET = 8,
	};

	static const std::function<void(const id::text_inserted_event&)> INSERTED;
	static const std::function<void(const id::text_replaced_event&)> REPLACED;
	static const std::function<void(const id::text_erased_event&)> ERASED;
	static const std::function<void(const id::text_set_event&)> SET;

	std::vector<id::connection> m_connections;

	void connect(int r, id::document& d)
	{
		if ((r & REJECT_INSERT) == REJECT_INSERT)
		{
			m_connections.push_back(d.text_inserted_signal.subscribe
			(INSERTED));
		}
		if ((r & REJECT_REPLACE) == REJECT_REPLACE)
		{
			m_connections.push_back(d.text_replaced_signal.subscribe
			(REPLACED));
		}
		if ((r & REJECT_ERASE) == REJECT_ERASE)
		{
			m_connections.push_back(d.text_erased_signal.subscribe
			(ERASED));
		}
		if ((r & REJECT_SET) == REJECT_SET)
		{
			m_connections.push_back(d.text_set_signal.subscribe
			(SET));
		}
	}

	void disconnect()
	{
		for (auto& connection : m_connections)
		{
			connection.disconnect();
		}
		m_connections.clear();
	}

}; // struct text_utilities
} } // namespace ego::test
