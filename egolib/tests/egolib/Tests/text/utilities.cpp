#include "egolib/Tests/text/utilities.hpp"

namespace ego { namespace test {

const std::function<void(const id::text_inserted_event&)> text_utilities::INSERTED =
	[](const id::text_inserted_event& e) { FAIL(); };

const std::function<void(const id::text_replaced_event&)> text_utilities::REPLACED =
	[](const id::text_replaced_event& e) { FAIL(); };

const std::function<void(const id::text_erased_event&)> text_utilities::ERASED =
	[](const id::text_erased_event& e) { FAIL(); };

const std::function<void(const id::text_set_event&)> text_utilities::SET =
[](const id::text_set_event& e) { FAIL(); };

} } // namespace ego::test