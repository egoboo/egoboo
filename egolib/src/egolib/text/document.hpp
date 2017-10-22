#pragma once

#include "egolib/platform.h"

namespace id {
	
// Forward declaration.
class text_erased_event;
class text_inserted_event;
class text_replaced_event;
class text_set_event;

/// @brief A policy which determins how the text of a document.
enum class document_trim_policy
{
	/// @brief Trim leading text.
	leading,
	/// @brief Trim trailing text.
	trailing,
}; // enum class document_trim_policy

/// @brief A document.
class document
{
public:
	/// @brief Construct this document.
	/// @remark
	/// The text is empty.
	/// The trim policy is id::document_trim_policy::trailing.
	/// There maximum length is std::numeric_limits<size_t>::max().
	document();

	/// @brief Destruct this document.
	virtual ~document();
	
	/// @brief Signal raised if text was erased.
	id::signal<void(const text_erased_event&)> text_erased_signal;
	
	/// @brief Signal raised if text was inserted.
	id::signal<void(const text_inserted_event&)> text_inserted_signal;
	
	/// @brief Signal raised if text was replaced.
	id::signal<void(const text_replaced_event&)> text_replaced_signal;

	/// @brief Signal raised if text was set.
	id::signal<void(const text_set_event&)> text_set_signal;

	/// @brief Erase a subtext.
	/// @param index the index of the subtext
	/// @except id::out_of_bounds_error @a index is not within the bounds of @a 0 and @a text.get_size().
	void erase(size_t index);

	/// @brief Erase a subtext.
	/// @param index the index of the subtext
	/// @param length the length of the subtext
	/// @except id::out_of_bounds_error @a index is not within the bounds of @a 0 and @a text.get_size().
	/// @except id::out_of_bounds_error @a (index + length) is not within the bounds of @a 0 and @a text.get_size().
	void erase(size_t index, size_t length);
	
	/// @brief Set the text.
	/// @param text the text
	void set_text(const std::string& text);
	
	/// @bief Append text.
	/// @param text the text
	void append_text(const std::string& text);
	
	/// @brief Prepend text.
	/// @param text the text
	void prepend_text(const std::string& text);

	/// @brief Insert text.
	/// @param text the text
	/// @param index the index at which the text is insered
	/// @throw id::out_of_bounds_error @a indx is greater than the length of the text
	void insert_text(const std::string& text, size_t index);

	/// @brief Get the trim policy of this document.
	/// @return the trim policy of this document
	id::document_trim_policy get_trim_policy() const;

	/// @brief Set the trim policy of this document.
	/// @param trim_policy the trim policy of this document
	void set_trim_policy(id::document_trim_policy trim_policy);

	/// @brief Get the maximum length of this document.
	/// @return the maximum length of this document
	size_t get_max_length() const;

	/// @brief Set the maximum length of this document.
	/// @param the maximum length of this document
	void set_max_length(size_t max_length);

public:
	/// @brief Get the text.
	/// @return the text
	std::string get_text() const;

private:
	void signal_text_set(const std::string& old_text, const std::string& new_text);
	void signal_text_erased(size_t index, size_t length, const std::string& old_text, const std::string& new_text);
	void signal_text_inserted(size_t index, size_t length, const std::string& old_text, const std::string& new_text);

	/// @brief See get_text() for more information.
	std::string m_text;

	/// @brief See get_trim_policy() for more information.
	document_trim_policy m_trim_policy;

	/// @brief See get_max_length() for more information.
	size_t m_max_length;

}; // struct text

} // namespace id
