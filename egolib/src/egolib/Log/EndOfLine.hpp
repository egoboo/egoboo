#pragma once

namespace Log {

/// @brief Manipulators influence the way characters, strings and entries are processed.
namespace Internal {

/// @internal
/// @brief The "end of line" manipulator type.
struct EndOfLineManipulation {};

} // namespace Internal

  /// @brief "end of line" manipulation.
extern const Internal::EndOfLineManipulation EndOfLine;

} // namespace Log
