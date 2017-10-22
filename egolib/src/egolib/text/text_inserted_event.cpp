#include "egolib/text/text_inserted_event.hpp"

namespace id {

text_inserted_event::text_inserted_event(size_t index, size_t length, const std::string& old_text, const std::string& new_text)
	: m_index(index), m_length(length), m_old_text(old_text), m_new_text(new_text)
{}

size_t text_inserted_event::get_index() const
{ return m_index; }

size_t text_inserted_event::get_length() const
{ return m_length; }

const std::string& text_inserted_event::get_old_text() const
{ return m_old_text; }

const std::string& text_inserted_event::get_new_text() const
{ return m_new_text; }
	
} // namespace id
