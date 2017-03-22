#pragma once

#include "egolib/Log/Target.hpp"
#include "egolib/Log/Level.hpp"
#include "egolib/Log/EndOfEntry.hpp"
#include "egolib/Log/EndOfLine.hpp"

namespace Log {

namespace Internal {

struct EntryImpl;

} // namespace Internal

/// Entries are not copyable, only movable.
struct Entry
{
private:
    /// The implementation of the entry.
    Internal::EntryImpl *impl;

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

    virtual ~Entry();

    Entry(Entry&& other);
    Entry& operator=(Entry&& other);

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
    friend Entry& operator<<(Entry& entry, const Internal::EndOfLineManipulation& value);
    friend Entry& operator<<(Entry& entry, const Internal::EndOfEntryManipulation& value);

private:
    /// @brief Write arguments into a stream.
    /// @param ostream the target output stream.
    /// @param head the head of the arguments list
    /// @param rest the rest of the arguments list
    /// @return the target output stream
    template <typename Arg, typename ... Args>
    static Entry& write(Entry& e, Arg&& arg, Args&& ... args)
    {
        e << arg;
        write(e, std::forward<Args>(args) ...);
        return e;
    }
    /// @brief Write arguments into a stream.
    /// @param ostream the target output stream
    /// @param arg the argument
    /// @return the target output stream
    template <typename Arg>
    static Entry& write(Entry& e, Arg&& arg)
    {
        e << arg;
        return e;
    }

public:
    /// @brief Create an entry.
    /// @param level, file, line the level, the file, and the line
    /// @return the entry
    template <typename ... Args>
    static Entry create(Level level, const std::string& file, int line, Args&& ... args)
    {
        Entry e(level, file, line);
        Entry::write(e, std::forward<Args>(args) ...);
        return e;
    }
    /// @brief Create a message.
    /// @param level, file, line the level, the file, and the line
    /// @return the entry
    static Entry create(Level level, const std::string& file, int line)
    {
        Entry e(level, file, line);
        return e;
    }
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
Entry& operator<<(Entry& entry, const Internal::EndOfLineManipulation& value);
Entry& operator<<(Entry& entry, const Internal::EndOfEntryManipulation& value);

Target& operator<<(Target& target, const Entry& entry);

} // namespace Log
