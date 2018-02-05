#include "egolib/text/text_set_event.hpp"

namespace idlib {

text_set_event::text_set_event(const std::string& old_text, const std::string& new_text)
	: m_old_text(old_text), m_new_text(new_text)
{}

const std::string& text_set_event::get_old_text() const
{ return m_old_text; }

const std::string& text_set_event::get_new_text() const
{ return m_new_text; }
	
} // namespace idlib
