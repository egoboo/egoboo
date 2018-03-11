#pragma once

namespace Log {

/// @brief Manipulators influence the way characters, strings and entries are processed.
namespace Internal {

/// @internal
/// @brief The "end of entry" manipulator type.
struct EndOfEntryManipulation {};

} // namespace Internal

/// @brief "end of entry" manipulation.
extern const Internal::EndOfEntryManipulation EndOfEntry;

} // namespace Log
