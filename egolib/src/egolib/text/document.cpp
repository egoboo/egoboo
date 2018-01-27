#include "egolib/text/document.hpp"

#include "egolib/text/text_erased_event.hpp"
#include "egolib/text/text_inserted_event.hpp"
#include "egolib/text/text_replaced_event.hpp"
#include "egolib/text/text_set_event.hpp"

namespace idlib {

document::document()
	: m_text(),
	  m_trim_policy(idlib::document_trim_policy::trailing),
	  m_max_length(std::numeric_limits<size_t>::max())
{}

document::~document()
{}

void document::erase(size_t index)
{
	if (index > m_text.size())
	{
		throw out_of_bounds_error(__FILE__, __LINE__, "index");
	}
	size_t length = m_text.size() - index;
	if (length == 0) return;
	auto old_text = m_text;
	m_text.erase(index);
	signal_text_erased(index, length, old_text, m_text);
}

void document::erase(size_t index, size_t length)
{
	if (index + length > m_text.size())
	{
		throw out_of_bounds_error(__FILE__, __LINE__, "index + length");
	}
	if (length == 0) return;
	auto old_text = m_text;
	m_text.erase(index, length);
	signal_text_erased(index, length, old_text, m_text);
}

void document::set_text(const std::string& text)
{
	if (m_text != text)
	{
		auto old_text = m_text,
			 new_text = text;
		if (new_text.size() > m_max_length)
		{
			size_t to_trim = new_text.size() - m_max_length;
			switch (m_trim_policy)
			{
				case document_trim_policy::leading:
				{
					new_text.erase(0, to_trim);
				} break;
				case document_trim_policy::trailing:
				{
					new_text.erase(new_text.size() - to_trim);
				} break;
			}; // switch
		}
		m_text = new_text;
		signal_text_set(old_text, new_text);
	}
}

void document::append_text(const std::string& text)
{
	insert_text(text, m_text.size());
}

void document::prepend_text(const std::string& text)
{
	insert_text(text, 0);
}

void document::signal_text_set(const std::string& old_text, const std::string& new_text)
{
	text_set_signal(text_set_event(old_text, new_text));
}

void document::signal_text_erased(size_t index, size_t length, const std::string& old_text, const std::string& new_text)
{
	text_erased_signal(text_erased_event(index, length, old_text, new_text));
}

void document::signal_text_inserted(size_t index, size_t length, const std::string& old_text, const std::string& new_text)
{
	text_inserted_signal(text_inserted_event(index, length, old_text, new_text));
}

void document::insert_text(const std::string& text, size_t index)
{
	if (index > m_text.size())
	{
		// if index is greater than the text size, then raise an error.
		throw idlib::out_of_bounds_error(__FILE__, __LINE__, "index");
	}
	if (text.empty())
	{
		// if the text to insert is empty, then there is nothin to do.
		return;
	}
	if (m_text.size() + text.size() <= m_max_length)
	{
		// if the new text does not exceed the maximum text length, there is nothing to do.
		auto old_text = m_text;
		m_text.insert(index, text);
		return signal_text_inserted(index, text.size(), old_text, m_text);
	}
	// the new text exceeds the maximum text length.
	size_t to_trim = m_text.size() + text.size() - m_max_length;
	size_t actual_index = index, actual_length = text.size();
	auto text_old = m_text, text_new = m_text;
	text_new.insert(index, text);
	switch (m_trim_policy)
	{
		case document_trim_policy::leading:
		{
			// The index is either in in the trimmed part or not.
			if (actual_index <= to_trim)
			{
				// The index is either in the trimmed part.
				text_new.erase(0, actual_index);
				actual_length -= actual_index;
				actual_index = 0;
			}
			else
			{
				// The index is not in the trimmed part.
				text_new.erase(0, to_trim);
				actual_index -= to_trim;
			}
		} break;
	case document_trim_policy::trailing:
		{
			if (index + text.size() > m_max_length)
			{
				size_t delta = index + text.size() - m_max_length;
				text_new.erase(m_max_length - delta, delta);
				actual_length -= delta;
			}
		} break;
	}; // switch
	m_text = text_new;
	signal_text_inserted(actual_index, actual_length, text_old, text_new);
}

idlib::document_trim_policy document::get_trim_policy() const
{
	return m_trim_policy;
}

void document::set_trim_policy(idlib::document_trim_policy trim_policy)
{
	m_trim_policy = trim_policy;
}

size_t document::get_max_length() const
{
	return m_max_length;
}

void document::set_max_length(size_t max_length)
{
	if (m_max_length != max_length)
	{
		m_max_length = max_length;
		if (m_text.size() > m_max_length)
		{
			size_t to_trim = m_text.size() - m_max_length;
			switch (m_trim_policy)
			{
			case idlib::document_trim_policy::leading:
				{
					auto old_text = m_text;
					m_text.erase(0, to_trim);
					signal_text_erased(0, to_trim, old_text, m_text);
				} break;
			case idlib::document_trim_policy::trailing:
				{
					auto old_text = m_text;
					m_text.erase(m_text.size() - to_trim);
					signal_text_erased(m_text.size() - to_trim, to_trim, old_text, m_text);
				} break;
			};
		}
	}
}

std::string document::get_text() const
{ return m_text; }

} // namespace id
