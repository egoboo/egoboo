#pragma once

#include "egolib/platform.h"

namespace id {

/// @brief Event raised if text was erased.
/// @a get_index() gets the starting index of the erased subtext (in the old text).
/// @a get_length() gets the length of the erased subtext.
/// @a get_old_text() gets the old text and @a get_new_text() gets the new text. 
/// The text that was erased can be obtained from an @a text_erased_event object @a e a as follows:
/// @code
/// auto s = e.get_old_text().begin() + e.get_index(),
///      e = e.get_old_text().begin() + e.get_index() + e.get_length();
/// @endcode
/// In particular, an empty text was erased if <c>s == e</c>/<c>e.ge_length() == 0</c>.
/// However, it is not recommended to raise a @a text_erased_event in that case.
struct text_erased_event
{
public:
	/// @param index the index of the erased subtext (in the old text)
	/// @param length the length of the erased text
	/// @param old_text the old text
	/// @param new_text the new text
	text_erased_event(size_t index, size_t length, const std::string& old_text, const std::string& new_text);

	/// @brief Get the index of the erased subtext (in the old text).
	/// @return the index of the erased subtext (in the old text)
	size_t get_index() const;

	/// @brief Get the length of the erased subtext (in the old text).
	/// @return the length of the erased subtext (in the old text)
	size_t get_length() const;

	/// @brief Get the old text.
	/// @return the old text
	const std::string& get_old_text() const;

	/// @brief Get the new text.
	/// @return the new text
	const std::string& get_new_text() const;

private:
	/// @internal
	/// @brief See index() for more information.
	size_t m_index;

	/// @internal
	/// @brief See length() for more information.
	size_t m_length;

	/// @internal
	/// @brief See get_old_text() for more information.
	std::string m_old_text;

	/// @internal
	/// @brief See get_new_text() for more information.
	std::string m_new_text;

}; // class text_erased_event

} // namespace id
