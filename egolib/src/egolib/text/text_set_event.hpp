#pragma once

#include "egolib/platform.h"

namespace id {

/// @brief Event raised if text was set.
/// @a get_old_text() gets the old text and @a get_new_text() gets the new text. 
/// However, it is not recommended to raise a @a text_set_event if <c>get_old_text() == get_new_text()</c>.
struct text_set_event
{
public:
	/// @param old_text the old text
	/// @param new_text the new text
	text_set_event(const std::string& old_text, const std::string& new_text);

	/// @brief Get the old text.
	/// @return the old text
	const std::string& get_old_text() const;

	/// @brief Get the new text.
	/// @return the new text
	const std::string& get_new_text() const;

private:
	/// @internal
	/// @brief See get_old_text() for more information.
	std::string m_old_text;

	/// @internal
	/// @brief See get_new_text() for more information.
	std::string m_new_text;

}; // class text_replaced_event

} // namespace id
