#pragma once

#include "egolib/platform.h"

namespace id {

/// @brief Event raised if text was inserted.
/// @a get_index() gets the index of the inserted subtext (in the new text).
/// @a get_length() gets length of the inserted subtext.
/// @a get_old_text() gets the old text and @a get_new_text() gets the new text. 
/// The text that was inserted can be obtained from an @a text_inserted_event object @a e a as follows:
/// @code
/// auto s = e.get_new_text().begin() + e.get_index(),
///      e = e.get_next_text().begin() + e.get_index() + e.get_length();
/// @endcode
/// In particular, an empty text was inserted if <c>s == e</c>/<c>e.ge_length() == 0</c>.
/// However, it is not recommended to raise a @a text_inserted_event in that case.
struct text_inserted_event
{
public:
	/// @brief Construct this text_inserted_event object.
	/// @param index the index of the inserted subtext (in the new text)
	/// @param length the length of the inserted subtext
	/// @param old_text the old text
	/// @param new_text the new text
	text_inserted_event(size_t index, size_t length, const std::string& old_text, const std::string& new_text);

	/// @brief Get the index of the inserted subtext (in the new text).
	/// @return the index of the inserted subtext (in the new text)
	size_t get_index() const;

	/// @brief Get the length of the inserted subtext.
	/// @return the length of the inserted subtext (in the new text)
	size_t get_length() const;

	/// @brief Get the old text.
	/// @return the old text
	const std::string& get_old_text() const;

	/// @brief Get the new text.
	/// @return the new text
	const std::string& get_new_text() const;
	
private:
	/// @brief See get_index() for more information.
	size_t m_index;

	/// @brief See get_length() for more information.
	size_t m_length;

	/// @brief See get_old_text() for more information.
	std::string m_old_text;

	/// @brief See get_new_text() for more information.
	std::string m_new_text;

}; // struct text_inserted_event

} // namespace id
