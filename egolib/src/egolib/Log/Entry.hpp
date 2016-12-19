#pragma once

#include "egolib/Log/Target.hpp"
#include "egolib/Log/Level.hpp"

namespace Log {

/// @brief Manipulators influence the way characters, strings and entries are processed.
namespace Manipulations {

/// @internal
/// @brief The "end of line" manipulator type.
struct EndOfLineManipulation {};

/// @internal
/// @brief The "end of entry" manipulator type.
struct EndOfEntryManipulation {};

}

/// @brief "end of entry" manipulation.
extern const Manipulations::EndOfEntryManipulation EndOfEntry;

/// @brief "end of line" manipulation.
extern const Manipulations::EndOfLineManipulation EndOfLine;

struct Entry
{
private:
    Level m_level;
    std::map<std::string, std::string> m_attributes;
    std::ostringstream m_sink;

public:
    /// @brief Construct this log entry.
    /// @param level the log level
    Entry(Level level);

    /// @brief Construct this log entry.
    /// @param level the log level
    /// @param fileName the C/C++ file name
    /// @param lineNumber the C/C++ line number
    Entry(Level level, const std::string& fileName, int lineNumber);

    /// @brief Construct this log entry.
    /// @param level the log level
    /// @param fileName the C/C++ file name
    /// @param lineNumber the C/C++ line number
    /// @param functionName the C/C++ function name
    Entry(Level level, const std::string& fileName, int lineNumber,
          const std::string& functionName);

    bool hasAttribute(const std::string& name) const;

    const std::string& getAttribute(const std::string& name) const;

    /// @brief Get the text of the log entry.
    /// @return the text of the log entry
    std::string getText() const;

    /// @brief Get the log level of the log entry.
    /// @return the log level of the log entry
    Level getLevel() const;

    friend Entry& operator<<(Entry& entry, const char *value);
    friend Entry& operator<<(Entry& entry, const std::string& value);
    friend Entry& operator<<(Entry& entry, bool value);
    friend Entry& operator<<(Entry& entry, char value);
    friend Entry& operator<<(Entry& entry, signed char value);
    friend Entry& operator<<(Entry& entry, unsigned char value);
    friend Entry& operator<<(Entry& entry, signed short value);
    friend Entry& operator<<(Entry& entry, unsigned short value);
    friend Entry& operator<<(Entry& entry, signed int value);
    friend Entry& operator<<(Entry& entry, unsigned int value);
    friend Entry& operator<<(Entry& entry, signed long value);
    friend Entry& operator<<(Entry& entry, unsigned long value);
    friend Entry& operator<<(Entry& entry, float value);
    friend Entry& operator<<(Entry& entry, double value);
    friend Entry& operator<<(Entry& entry, signed long long value);
    friend Entry& operator<<(Entry& entry, unsigned long long value);
    friend Entry& operator<<(Entry& entry, const Manipulations::EndOfLineManipulation& value);
    friend Entry& operator<<(Entry& entry, const Manipulations::EndOfEntryManipulation& value);
};

Entry& operator<<(Entry& entry, const char *value);
Entry& operator<<(Entry& entry, const std::string& value);
Entry& operator<<(Entry& entry, char value);
Entry& operator<<(Entry& entry, signed char value);
Entry& operator<<(Entry& entry, unsigned char value);
Entry& operator<<(Entry& entry, signed short value);
Entry& operator<<(Entry& entry, unsigned short value);
Entry& operator<<(Entry& entry, bool value);
Entry& operator<<(Entry& entry, signed int value);
Entry& operator<<(Entry& entry, unsigned int value);
Entry& operator<<(Entry& entry, signed long value);
Entry& operator<<(Entry& entry, unsigned long value);
Entry& operator<<(Entry& entry, float value);
Entry& operator<<(Entry& entry, double value);
Entry& operator<<(Entry& entry, signed long long value);
Entry& operator<<(Entry& entry, unsigned long long value);
Entry& operator<<(Entry& entry, const Manipulations::EndOfLineManipulation& value);
Entry& operator<<(Entry& entry, const Manipulations::EndOfEntryManipulation& value);

Target& operator<<(Target& target, const Entry& entry);

} // namespace Log
